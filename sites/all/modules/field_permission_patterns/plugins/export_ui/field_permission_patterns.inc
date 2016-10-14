<?php
/**
 * @file
 *  Ctools Exportables UI configuration.
 */

$plugin = array(
  'schema' => 'field_permission_patterns',
  'access' => 'administer field permissions',

  'menu' => array(
    'menu item' => 'field-permission-patterns',
    'menu title' => 'Field permission patterns',
    'menu description' => 'Field permission patterns',
  ),

  'title singular' => t('pattern'),
  'title singular proper' => t('Pattern'),
  'title plural' => t('patterns'),
  'title plural proper' => t('Patterns'),

  'handler' => 'field_permission_patterns_ui',
);
