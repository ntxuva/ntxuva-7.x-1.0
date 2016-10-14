<?php
/**
 * @file
 * voipscriptui2.features.inc
 */

/**
 * Implements hook_node_info().
 */
function voipscriptui_node_info() {
  $items = array(
    'voipscriptui' => array(
      'name' => t('VoIP Script'),
      'base' => 'node_content',
      'description' => t('Dialplan script used in Voip Drupal calls.'),
      'has_title' => '1',
      'title_label' => t('Script name'),
      'help' => '',
    ),
  );
  return $items;
}
