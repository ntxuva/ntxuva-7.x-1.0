<?php

/**
 * @file
 *  Provide sample VoIP all log table.
 */

/**
 * Implements hook_views_default_views().
 */
function voipcall_views_default_views() {
  $views = array();
  $view = new view;
  $view->name = 'voipcall';
  $view->description = 'VoIP call log';
  $view->tag = '';
  $view->base_table = 'voipcall';
  $view->human_name = '';
  $view->core = 0;
  $view->api_version = '3.0';
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

  /* Display: Defaults */
  $handler = $view->new_display('default', 'Defaults', 'default');
  $handler->display->display_options['title'] = 'VoIP Call Log';
  $handler->display->display_options['access']['type'] = 'perm';
  $handler->display->display_options['access']['perm'] = 'administer voip drupal framework';
  $handler->display->display_options['cache']['type'] = 'none';
  $handler->display->display_options['query']['type'] = 'views_query';
  $handler->display->display_options['query']['options']['query_comment'] = FALSE;
  $handler->display->display_options['exposed_form']['type'] = 'basic';
  $handler->display->display_options['pager']['type'] = 'full';
  $handler->display->display_options['pager']['options']['items_per_page'] = 50;
  $handler->display->display_options['style_plugin'] = 'table';
  $handler->display->display_options['style_options']['grouping'] = '';
  $handler->display->display_options['style_options']['columns'] = array(
    'call_channel' => 'call_channel',
    'call_network' => 'call_network',
    'caller_name' => 'caller_name',
    'caller_number' => 'caller_number',
    'dest_name' => 'dest_name',
    'dest_number' => 'dest_number',
    'direction' => 'direction',
    'duration' => 'duration',
    'start_time' => 'start_time',
    'status' => 'status',
  );
  $handler->display->display_options['style_options']['default'] = '-1';
  $handler->display->display_options['style_options']['info'] = array(
    'call_channel' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'call_network' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'caller_name' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'caller_number' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'dest_name' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'dest_number' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'direction' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'duration' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'start_time' => array(
      'sortable' => 1,
      'separator' => '',
    ),
    'status' => array(
      'sortable' => 1,
      'separator' => '',
    ),
  );
  $handler->display->display_options['style_options']['override'] = 1;
  $handler->display->display_options['style_options']['sticky'] = 0;
  /* Relationship: VoIP call: user */
  $handler->display->display_options['relationships']['uid']['id'] = 'uid';
  $handler->display->display_options['relationships']['uid']['table'] = 'voipcall';
  $handler->display->display_options['relationships']['uid']['field'] = 'uid';
  $handler->display->display_options['relationships']['uid']['required'] = 0;
  /* Field: VoIP call: VoIP call serial ID */
  $handler->display->display_options['fields']['cid']['id'] = 'cid';
  $handler->display->display_options['fields']['cid']['table'] = 'voipcall';
  $handler->display->display_options['fields']['cid']['field'] = 'cid';
  $handler->display->display_options['fields']['cid']['label'] = 'CID';
  $handler->display->display_options['fields']['cid']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['cid']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['cid']['alter']['absolute'] = 0;
  $handler->display->display_options['fields']['cid']['alter']['external'] = 0;
  $handler->display->display_options['fields']['cid']['alter']['replace_spaces'] = 0;
  $handler->display->display_options['fields']['cid']['alter']['trim_whitespace'] = 0;
  $handler->display->display_options['fields']['cid']['alter']['nl2br'] = 0;
  $handler->display->display_options['fields']['cid']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['cid']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['cid']['alter']['more_link'] = 0;
  $handler->display->display_options['fields']['cid']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['cid']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['cid']['alter']['html'] = 0;
  $handler->display->display_options['fields']['cid']['element_label_colon'] = 1;
  $handler->display->display_options['fields']['cid']['element_default_classes'] = 1;
  $handler->display->display_options['fields']['cid']['hide_empty'] = 0;
  $handler->display->display_options['fields']['cid']['empty_zero'] = 0;
  $handler->display->display_options['fields']['cid']['hide_alter_empty'] = 1;
  $handler->display->display_options['fields']['cid']['separator'] = '';
  $handler->display->display_options['fields']['cid']['format_plural'] = 0;
  /* Field: VoIP call: Start time */
  $handler->display->display_options['fields']['start_time']['id'] = 'start_time';
  $handler->display->display_options['fields']['start_time']['table'] = 'voipcall';
  $handler->display->display_options['fields']['start_time']['field'] = 'start_time';
  $handler->display->display_options['fields']['start_time']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['start_time']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['start_time']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['start_time']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['start_time']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['start_time']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['start_time']['alter']['html'] = 0;
  $handler->display->display_options['fields']['start_time']['hide_empty'] = 0;
  $handler->display->display_options['fields']['start_time']['empty_zero'] = 0;
  /* Field: User: Name */
  $handler->display->display_options['fields']['name']['id'] = 'name';
  $handler->display->display_options['fields']['name']['table'] = 'users';
  $handler->display->display_options['fields']['name']['field'] = 'name';
  $handler->display->display_options['fields']['name']['relationship'] = 'uid';
  $handler->display->display_options['fields']['name']['label'] = 'User';
  $handler->display->display_options['fields']['name']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['name']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['name']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['name']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['name']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['name']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['name']['alter']['html'] = 0;
  $handler->display->display_options['fields']['name']['hide_empty'] = 0;
  $handler->display->display_options['fields']['name']['empty_zero'] = 0;
  $handler->display->display_options['fields']['name']['link_to_user'] = 1;
  $handler->display->display_options['fields']['name']['overwrite_anonymous'] = 0;
  /* Field: VoIP call: Caller name */
  $handler->display->display_options['fields']['caller_name']['id'] = 'caller_name';
  $handler->display->display_options['fields']['caller_name']['table'] = 'voipcall';
  $handler->display->display_options['fields']['caller_name']['field'] = 'caller_name';
  $handler->display->display_options['fields']['caller_name']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['caller_name']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['caller_name']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['caller_name']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['caller_name']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['caller_name']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['caller_name']['alter']['html'] = 0;
  $handler->display->display_options['fields']['caller_name']['hide_empty'] = 0;
  $handler->display->display_options['fields']['caller_name']['empty_zero'] = 0;
  /* Field: VoIP call: Caller number */
  $handler->display->display_options['fields']['caller_number']['id'] = 'caller_number';
  $handler->display->display_options['fields']['caller_number']['table'] = 'voipcall';
  $handler->display->display_options['fields']['caller_number']['field'] = 'caller_number';
  $handler->display->display_options['fields']['caller_number']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['caller_number']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['caller_number']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['caller_number']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['caller_number']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['caller_number']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['caller_number']['alter']['html'] = 0;
  $handler->display->display_options['fields']['caller_number']['hide_empty'] = 0;
  $handler->display->display_options['fields']['caller_number']['empty_zero'] = 0;
  /* Field: VoIP call: Destination name */
  $handler->display->display_options['fields']['dest_name']['id'] = 'dest_name';
  $handler->display->display_options['fields']['dest_name']['table'] = 'voipcall';
  $handler->display->display_options['fields']['dest_name']['field'] = 'dest_name';
  $handler->display->display_options['fields']['dest_name']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['dest_name']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['dest_name']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['dest_name']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['dest_name']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['dest_name']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['dest_name']['alter']['html'] = 0;
  $handler->display->display_options['fields']['dest_name']['hide_empty'] = 0;
  $handler->display->display_options['fields']['dest_name']['empty_zero'] = 0;
  /* Field: VoIP call: Destination number */
  $handler->display->display_options['fields']['dest_number']['id'] = 'dest_number';
  $handler->display->display_options['fields']['dest_number']['table'] = 'voipcall';
  $handler->display->display_options['fields']['dest_number']['field'] = 'dest_number';
  $handler->display->display_options['fields']['dest_number']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['dest_number']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['dest_number']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['dest_number']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['dest_number']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['dest_number']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['dest_number']['alter']['html'] = 0;
  $handler->display->display_options['fields']['dest_number']['hide_empty'] = 0;
  $handler->display->display_options['fields']['dest_number']['empty_zero'] = 0;
  /* Field: Global: Custom text */
  $handler->display->display_options['fields']['nothing']['id'] = 'nothing';
  $handler->display->display_options['fields']['nothing']['table'] = 'views';
  $handler->display->display_options['fields']['nothing']['field'] = 'nothing';
  $handler->display->display_options['fields']['nothing']['label'] = 'Other';
  $handler->display->display_options['fields']['nothing']['alter']['text'] = 'See more';
  $handler->display->display_options['fields']['nothing']['alter']['make_link'] = 1;
  $handler->display->display_options['fields']['nothing']['alter']['path'] = 'voipcall/[cid]';
  $handler->display->display_options['fields']['nothing']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['nothing']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['nothing']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['nothing']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['nothing']['alter']['html'] = 0;
  $handler->display->display_options['fields']['nothing']['hide_empty'] = 0;
  $handler->display->display_options['fields']['nothing']['empty_zero'] = 0;
  /* Sort criterion: VoIP call: VoIP call serial ID */
  $handler->display->display_options['sorts']['cid']['id'] = 'cid';
  $handler->display->display_options['sorts']['cid']['table'] = 'voipcall';
  $handler->display->display_options['sorts']['cid']['field'] = 'cid';
  $handler->display->display_options['sorts']['cid']['order'] = 'DESC';

  /* Display: Page */
  $handler = $view->new_display('page', 'Page', 'page_1');
  $handler->display->display_options['path'] = 'admin/voip/call/log';
  $handler->display->display_options['menu']['type'] = 'normal';
  $handler->display->display_options['menu']['title'] = 'VoIP Call Log';
  $handler->display->display_options['menu']['weight'] = '0';
  $handler->display->display_options['menu']['name'] = 'management';
  $handler->display->display_options['menu']['context'] = 0;

  $views[$view->name] = $view;

  return $views;
}
