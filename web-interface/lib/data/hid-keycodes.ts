export interface HIDKeycode {
  key: string;
  code: number;
  category: string;
}

export const HID_KEYCODES: HIDKeycode[] = [
  // Letters
  { key: 'A', code: 4, category: 'Letters' },
  { key: 'B', code: 5, category: 'Letters' },
  { key: 'C', code: 6, category: 'Letters' },
  { key: 'D', code: 7, category: 'Letters' },
  { key: 'E', code: 8, category: 'Letters' },
  { key: 'F', code: 9, category: 'Letters' },
  { key: 'G', code: 10, category: 'Letters' },
  { key: 'H', code: 11, category: 'Letters' },
  { key: 'I', code: 12, category: 'Letters' },
  { key: 'J', code: 13, category: 'Letters' },
  { key: 'K', code: 14, category: 'Letters' },
  { key: 'L', code: 15, category: 'Letters' },
  { key: 'M', code: 16, category: 'Letters' },
  { key: 'N', code: 17, category: 'Letters' },
  { key: 'O', code: 18, category: 'Letters' },
  { key: 'P', code: 19, category: 'Letters' },
  { key: 'Q', code: 20, category: 'Letters' },
  { key: 'R', code: 21, category: 'Letters' },
  { key: 'S', code: 22, category: 'Letters' },
  { key: 'T', code: 23, category: 'Letters' },
  { key: 'U', code: 24, category: 'Letters' },
  { key: 'V', code: 25, category: 'Letters' },
  { key: 'W', code: 26, category: 'Letters' },
  { key: 'X', code: 27, category: 'Letters' },
  { key: 'Y', code: 28, category: 'Letters' },
  { key: 'Z', code: 29, category: 'Letters' },

  // Numbers
  { key: '1', code: 30, category: 'Numbers' },
  { key: '2', code: 31, category: 'Numbers' },
  { key: '3', code: 32, category: 'Numbers' },
  { key: '4', code: 33, category: 'Numbers' },
  { key: '5', code: 34, category: 'Numbers' },
  { key: '6', code: 35, category: 'Numbers' },
  { key: '7', code: 36, category: 'Numbers' },
  { key: '8', code: 37, category: 'Numbers' },
  { key: '9', code: 38, category: 'Numbers' },
  { key: '0', code: 39, category: 'Numbers' },

  // Special Keys
  { key: 'Enter', code: 40, category: 'Special' },
  { key: 'Escape', code: 41, category: 'Special' },
  { key: 'Backspace', code: 42, category: 'Special' },
  { key: 'Tab', code: 43, category: 'Special' },
  { key: 'Space', code: 44, category: 'Special' },
  { key: 'Caps Lock', code: 57, category: 'Special' },

  // Function Keys
  { key: 'F1', code: 58, category: 'Function' },
  { key: 'F2', code: 59, category: 'Function' },
  { key: 'F3', code: 60, category: 'Function' },
  { key: 'F4', code: 61, category: 'Function' },
  { key: 'F5', code: 62, category: 'Function' },
  { key: 'F6', code: 63, category: 'Function' },
  { key: 'F7', code: 64, category: 'Function' },
  { key: 'F8', code: 65, category: 'Function' },
  { key: 'F9', code: 66, category: 'Function' },
  { key: 'F10', code: 67, category: 'Function' },
  { key: 'F11', code: 68, category: 'Function' },
  { key: 'F12', code: 69, category: 'Function' },

  // Navigation
  { key: 'Print Screen', code: 70, category: 'Navigation' },
  { key: 'Scroll Lock', code: 71, category: 'Navigation' },
  { key: 'Pause', code: 72, category: 'Navigation' },
  { key: 'Insert', code: 73, category: 'Navigation' },
  { key: 'Home', code: 74, category: 'Navigation' },
  { key: 'Page Up', code: 75, category: 'Navigation' },
  { key: 'Delete', code: 76, category: 'Navigation' },
  { key: 'End', code: 77, category: 'Navigation' },
  { key: 'Page Down', code: 78, category: 'Navigation' },
  { key: 'Right Arrow', code: 79, category: 'Navigation' },
  { key: 'Left Arrow', code: 80, category: 'Navigation' },
  { key: 'Down Arrow', code: 81, category: 'Navigation' },
  { key: 'Up Arrow', code: 82, category: 'Navigation' },

  // Modifiers
  { key: 'Left Ctrl', code: 224, category: 'Modifiers' },
  { key: 'Left Shift', code: 225, category: 'Modifiers' },
  { key: 'Left Alt', code: 226, category: 'Modifiers' },
  { key: 'Left GUI (Win)', code: 227, category: 'Modifiers' },
  { key: 'Right Ctrl', code: 228, category: 'Modifiers' },
  { key: 'Right Shift', code: 229, category: 'Modifiers' },
  { key: 'Right Alt', code: 230, category: 'Modifiers' },
  { key: 'Right GUI (Win)', code: 231, category: 'Modifiers' },
];

export const KEYCODE_CATEGORIES = [
  'Letters',
  'Numbers',
  'Function',
  'Special',
  'Navigation',
  'Modifiers',
];
