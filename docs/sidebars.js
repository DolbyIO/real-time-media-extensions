/**
 * Creating a sidebar enables you to:
 - create an ordered group of docs
 - render a sidebar for each doc of that group
 - provide next/previous navigation

 The sidebars can be generated from the filesystem, or explicitly defined here.

 Create as many sidebars as you want.
 */

// @ts-check

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */

module.exports = {
  sidebar: [
    {
      type: 'doc',
      id: 'introduction'
    },
    {
      type: 'category',
      label: 'Installing and Executing',
      link: {
        type: 'generated-index',
        description: 'You can install and run Real-time Media Extensions in two ways; you can either build it and run it on Linux or use a Docker container.'
      },
      items: [
        'Installing and executing/docker-container',
        {
          type: 'category',
          label: 'Linux',
          link: {
            type: 'generated-index',
            description: 'Before running Real-time Media Extensions on Linux, build it or use the Linux executable.'
          },
          items: [
            'Installing and executing/Linux/building',
            'Installing and executing/Linux/modifying-components',
            'Installing and executing/Linux/running-natively',
          ],
        },
      ],
    },
    {
      type: 'doc',
      id: 'configuration-parameters'
    },
    {
      type: 'category',
      label: 'Applications',
      items: [
        {
          type: 'category',
          label: 'Live Transcription',
          link: {
            type: "doc",
            id: 'Applications/live-transcription'
          },
          items: [
            'Applications/Live Transcription/aws-transcribe',
            'Applications/Live Transcription/gladia-io'
          ],
        }
      ]
    },
  ],
};