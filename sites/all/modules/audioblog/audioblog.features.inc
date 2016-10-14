<?php
/**
 * @file
 * audioblog.features.inc
 */

/**
 * Implements hook_node_info().
 */
function audioblog_node_info() {
  $items = array(
    'audioblog' => array(
      'name' => t('Audio Blog'),
      'base' => 'node_content',
      'description' => '',
      'has_title' => '1',
      'title_label' => t('Title'),
      'help' => '',
    ),
  );
  return $items;
}
