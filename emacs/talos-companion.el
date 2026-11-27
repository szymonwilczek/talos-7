;;; talos-companion.el --- Evil-friendly Talos 7 Companion bridge -*- lexical-binding: t; -*-

;; Copyright (C) 2026 Szymon Wilczek
;; SPDX-License-Identifier: GPL-3.0-or-later

;;; Commentary:
;; Pushes active Emacs buffer/major-mode changes, throttled typing pulses,
;; and Magit commit notifications to the local talos-bridge Unix domain socket.
;; Deliberately omits `after-save-hook' (for frequent Evil `:w' saves) and
;; `compilation-finish-functions' (builds run via agents/CI).

;;; Code:

(require 'subr-x)

(defgroup talos-companion nil
  "Talos 7 Companion Mode integration for Emacs."
  :group 'convenience
  :prefix "talos-companion-")

(defcustom talos-companion-socket-path nil
  "Optional explicit path to the talos-bridge Unix domain socket.
When nil, resolves `$TALOS_BRIDGE_SOCK', `$XDG_RUNTIME_DIR/talos-bridge.sock',
or `/tmp/talos-bridge-$UID.sock'."
  :type '(choice (const :tag "Auto-detect" nil) file)
  :group 'talos-companion)

(defcustom talos-companion-typing-throttle-seconds 3.0
  "Minimum seconds between consecutive `typing' notifications sent to bridge."
  :type 'number
  :group 'talos-companion)

(defcustom talos-companion-reconnect-backoff-seconds 2.0
  "Minimum seconds before retrying a failed Unix socket connection."
  :type 'number
  :group 'talos-companion)

(defvar talos-companion--proc nil)
(defvar talos-companion--last-typing-time 0.0)
(defvar talos-companion--last-connect-fail 0.0)
(defvar talos-companion--last-buffer-line nil)

(defvar talos-companion-send-function #'talos-companion--send-via-socket
  "Transport function invoked with a single formatted protocol line.")

(defun talos-companion-socket-path ()
  "Return the resolved local Unix domain socket path for `talos-bridge'."
  (or talos-companion-socket-path
      (let ((env-sock (getenv "TALOS_BRIDGE_SOCK")))
        (when (and env-sock (not (string-empty-p env-sock)))
          env-sock))
      (let ((runtime-dir (getenv "XDG_RUNTIME_DIR")))
        (if (and runtime-dir (not (string-empty-p runtime-dir)))
            (expand-file-name "talos-bridge.sock" runtime-dir)
          (format "/tmp/talos-bridge-%d.sock" (user-uid))))))

(defun talos-companion--disconnect ()
  "Close any active local bridge socket process safely."
  (when (processp talos-companion--proc)
    (ignore-errors (delete-process talos-companion--proc)))
  (setq talos-companion--proc nil))

(defun talos-companion--ensure-process ()
  "Return a live non-blocking Unix socket process, or nil if unavailable."
  (when (and (processp talos-companion--proc)
             (not (memq (process-status talos-companion--proc) '(open run connect))))
    (talos-companion--disconnect))
  (or talos-companion--proc
      (let ((now (float-time))
            (sock (talos-companion-socket-path)))
        (when (and sock
                   (file-exists-p sock)
                   (>= (- now talos-companion--last-connect-fail)
                       talos-companion-reconnect-backoff-seconds))
          (condition-case nil
              (let ((proc (make-network-process
                           :name "talos-bridge"
                           :family 'local
                           :service sock
                           :nowait t
                           :noquery t
                           :sentinel (lambda (p _ev)
                                       (when (not (memq (process-status p)
                                                        '(open run connect)))
                                         (when (eq p talos-companion--proc)
                                           (setq talos-companion--proc nil)))))))
                (setq talos-companion--proc proc))
            (error
             (setq talos-companion--last-connect-fail now)
             nil))))))

(defun talos-companion--sanitize-field (text)
  "Sanitize TEXT into a single line without pipe delimiters."
  (let* ((s (or text ""))
         (s (replace-regexp-in-string "|" "/" s))
         (s (replace-regexp-in-string "[\r\n]+" " " s)))
    (string-trim s)))

(defun talos-companion--send-via-socket (line)
  "Write LINE non-blockingly to `talos-bridge' if its Unix socket is present."
  (let ((proc (talos-companion--ensure-process)))
    (when proc
      (condition-case nil
          (progn
            (process-send-string proc (if (string-suffix-p "\n" line)
                                          line
                                        (concat line "\n")))
            t)
        (error
         (setq talos-companion--last-connect-fail (float-time))
         (talos-companion--disconnect)
         nil)))))

(defun talos-companion-format-buffer-line (&optional buffer)
  "Format an `emacs-buf|<buffer>|<mode>' line for BUFFER (or current buffer)."
  (with-current-buffer (or buffer (current-buffer))
    (let* ((buf-name (talos-companion--sanitize-field (buffer-name)))
           (mode-raw (symbol-name major-mode))
           (mode-short (replace-regexp-in-string "-mode\\'" "" mode-raw)))
      (format "emacs-buf|%s|%s" buf-name
              (talos-companion--sanitize-field mode-short)))))

(defun talos-companion-send-event (event &optional label)
  "Send companion EVENT with optional LABEL to the running host bridge."
  (let ((clean-evt (talos-companion--sanitize-field event))
        (clean-lbl (and label (talos-companion--sanitize-field label))))
    (funcall talos-companion-send-function
             (if (and clean-lbl (not (string-empty-p clean-lbl)))
                 (format "%s|%s" clean-evt clean-lbl)
               clean-evt))))

(defun talos-companion--push-buffer (&rest _args)
  "Send current buffer context if it changed since the last update."
  (when talos-companion-mode
    (let ((line (talos-companion-format-buffer-line (window-buffer (selected-window)))))
      (unless (equal line talos-companion--last-buffer-line)
        (when (funcall talos-companion-send-function line)
          (setq talos-companion--last-buffer-line line))))))

(defun talos-companion--on-self-insert ()
  "Send a throttled typing pulse to wake Mochi typing feedback."
  (when talos-companion-mode
    (let ((now (float-time)))
      (when (>= (- now talos-companion--last-typing-time)
                talos-companion-typing-throttle-seconds)
        (when (funcall talos-companion-send-function "typing")
          (setq talos-companion--last-typing-time now))))))

(defun talos-companion--on-magit-commit ()
  "Notify bridge when a Git commit finishes cleanly in Magit."
  (when talos-companion-mode
    (let* ((root (directory-file-name (expand-file-name default-directory)))
           (repo (file-name-nondirectory root)))
      (talos-companion-send-event "git-commit" repo))))

;;;###autoload
(define-minor-mode talos-companion-mode
  "Toggle lightweight Talos 7 Companion Mode host context reporting."
  :global t
  :init-value nil
  :lighter " Talos"
  (if talos-companion-mode
      (progn
        (setq talos-companion--last-buffer-line nil)
        (add-hook 'post-self-insert-hook #'talos-companion--on-self-insert)
        (add-hook 'window-selection-change-functions #'talos-companion--push-buffer)
        (add-hook 'window-buffer-change-functions #'talos-companion--push-buffer)
        (add-hook 'magit-post-commit-hook #'talos-companion--on-magit-commit)
        (talos-companion--push-buffer))
    (remove-hook 'post-self-insert-hook #'talos-companion--on-self-insert)
    (remove-hook 'window-selection-change-functions #'talos-companion--push-buffer)
    (remove-hook 'window-buffer-change-functions #'talos-companion--push-buffer)
    (remove-hook 'magit-post-commit-hook #'talos-companion--on-magit-commit)
    (setq talos-companion--last-buffer-line nil)
    (talos-companion--disconnect)))

(provide 'talos-companion)
;;; talos-companion.el ends here
