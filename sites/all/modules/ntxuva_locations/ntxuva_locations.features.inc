<?php
/**
 * @file
 * ntxuva_locations.features.inc
 */

/**
 * Implements hook_ctools_plugin_api().
 */
function ntxuva_locations_ctools_plugin_api($module = NULL, $api = NULL) {
  if ($module == "strongarm" && $api == "strongarm") {
    return array("version" => "1");
  }
}

/**
 * Implements hook_node_info().
 */
function ntxuva_locations_node_info() {
  $items = array(
    'locations' => array(
      'name' => t('Locations'),
      'base' => 'node_content',
      'description' => t('This content type contains all locations that will be used by the USSD and VoIP Drupal services. 

It will include all locations where users without Smartphone or Web Access will be able to submit new requests.'),
      'has_title' => '1',
      'title_label' => t('Location_id'),
      'help' => '',
    ),
  );
  return $items;
}
