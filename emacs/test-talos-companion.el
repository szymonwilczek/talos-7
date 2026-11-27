;;; test-talos-companion.el --- ERT tests for talos-companion.el -*- lexical-binding: t; -*-

(require 'ert)
(require 'cl-lib)
(require 'talos-companion)

(ert-deftest talos-companion-test-format-buffer ()
  "Buffer lines sanitize pipes and trim `-mode' suffix."
  (with-temp-buffer
    (rename-buffer "dev|notes.c" t)
    (let ((major-mode 'c-mode))
      (should (equal (talos-companion-format-buffer-line (current-buffer))
                     "emacs-buf|dev/notes.c|c")))))

(ert-deftest talos-companion-test-typing-throttle ()
  "Typing pulses respect the configured throttle cooldown."
  (let* ((sent '())
         (fake-time 100.0)
         (talos-companion-mode t)
         (talos-companion-typing-throttle-seconds 3.0)
         (talos-companion--last-typing-time 0.0)
         (talos-companion-send-function
          (lambda (line)
            (push line sent)
            t)))
    (cl-letf (((symbol-function 'float-time) (lambda (&optional _t) fake-time)))
      (talos-companion--on-self-insert)
      (setq fake-time 101.0)
      (talos-companion--on-self-insert)
      (setq fake-time 103.5)
      (talos-companion--on-self-insert))
    (should (equal (nreverse sent) '("typing" "typing")))))

(ert-deftest talos-companion-test-no-save-or-compile-hooks ()
  "Evil frequent `:w' saves and compilation hooks must not be attached."
  (let ((after-save-hook nil)
        (compilation-finish-functions nil)
        (talos-companion-send-function (lambda (_line) nil)))
    (unwind-protect
        (progn
          (talos-companion-mode 1)
          (should-not (cl-some (lambda (fn)
                                 (string-prefix-p "talos-companion"
                                                  (symbol-name fn)))
                               after-save-hook))
          (should-not (cl-some (lambda (fn)
                                 (string-prefix-p "talos-companion"
                                                  (symbol-name fn)))
                               compilation-finish-functions)))
      (talos-companion-mode -1))))

(ert-deftest talos-companion-test-missing-socket-safe ()
  "Sending events without a running daemon fails silently without signaling."
  (let ((talos-companion-socket-path "/tmp/talos-missing-socket-test-99999.sock")
        (talos-companion--proc nil)
        (talos-companion--last-connect-fail 0.0)
        (talos-companion-mode t))
    (should-not (talos-companion-send-event "ping" "Check"))
    (should-not (talos-companion--on-self-insert))))

(provide 'test-talos-companion)
;;; test-talos-companion.el ends here
