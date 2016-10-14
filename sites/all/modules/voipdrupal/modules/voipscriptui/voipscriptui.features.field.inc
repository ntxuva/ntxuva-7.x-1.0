<?php
/**
 * @file
 * voipscriptui2.features.field.inc
 */

/**
 * Implements hook_field_default_fields().
 */
function voipscriptui_field_default_fields() {
  $fields = array();

  // Exported field: 'node-voipscriptui-field_voipscriptui_code'
  $fields['node-voipscriptui-field_voipscriptui_code'] = array(
    'field_config' => array(
      'active' => '1',
      'cardinality' => '1',
      'deleted' => '0',
      'entity_types' => array(),
      'field_name' => 'field_voipscriptui_code',
      'foreign keys' => array(
        'format' => array(
          'columns' => array(
            'format' => 'format',
          ),
          'table' => 'filter_format',
        ),
      ),
      'indexes' => array(
        'format' => array(
          0 => 'format',
        ),
      ),
      'module' => 'text',
      'settings' => array(),
      'translatable' => '0',
      'type' => 'text_long',
    ),
    'field_instance' => array(
      'bundle' => 'voipscriptui',
      'default_value' => array(
        0 => array(
          'value' => '$script->addSay(\'your message here\');
$script->addHangup();',
        ),
      ),
      'deleted' => '0',
      'description' => 'Write your script above. The variable <i>$script</i> is automatically provided.  The script will be returned whenever a module requests its name via <i>hook_voipscript_load_script($script_name)</i>. To access the parameters passed to the script, use the predefined <i>$options</i> array variable.',
      'display' => array(
        'default' => array(
          'label' => 'above',
          'module' => 'text',
          'settings' => array(),
          'type' => 'text_default',
          'weight' => 0,
        ),
        'teaser' => array(
          'label' => 'above',
          'settings' => array(),
          'type' => 'hidden',
          'weight' => 0,
        ),
      ),
      'entity_type' => 'node',
      'field_name' => 'field_voipscriptui_code',
      'label' => 'Script Code',
      'required' => 1,
      'settings' => array(
        'text_processing' => '0',
        'user_register_form' => FALSE,
      ),
      'widget' => array(
        'active' => 1,
        'module' => 'text',
        'settings' => array(
          'rows' => '20',
        ),
        'type' => 'text_textarea',
        'weight' => '-4',
      ),
    ),
  );

  // Translatables
  // Included for use with string extractors like potx.
  t('Script Code');
  t('Write your script above. The variable <i>$script</i> is automatically provided.  The script will be returned whenever a module requests its name via <i>hook_voipscript_load_script($script_name)</i>. To access the parameters passed to the script, use the predefined <i>$options</i> array variable.');

  return $fields;
}
