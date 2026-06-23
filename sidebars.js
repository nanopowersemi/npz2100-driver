/**
 * sidebars.js — nPZ2100 driver documentation sidebar configuration.
 *
 * Drop this object into your Docusaurus sidebars.js (or sidebars.ts) under
 * the key that matches the `id` values declared in each .mdx front-matter.
 *
 * Usage in sidebars.js:
 * ---------------------
 *   const { npz2100Sidebar } = require('./npz2100/docs/sidebars.js');
 *
 *   module.exports = {
 *     mySidebar: [
 *       ...npz2100Sidebar,
 *       // ...other sidebar items
 *     ],
 *   };
 *
 * Or merge it directly if you prefer:
 *   module.exports = {
 *     mySidebar: npz2100Sidebar,
 *   };
 */

const npz2100Sidebar = [
  {
    type: 'category',
    label: 'nPZ2100 Driver',
    collapsible: true,
    collapsed: false,
    items: [
      // Overview and quick-start (overview.mdx)
      'overview',

      // HAL interface and register primitive API (hal.mdx)
      'hal',

      // Full register reference by functional block (registers.mdx)
      'registers',

      // Mid-level API: register map, shadow, typed helpers (mid-level.mdx)
      'mid-level',
    ],
  },
];

module.exports = { npz2100Sidebar };
