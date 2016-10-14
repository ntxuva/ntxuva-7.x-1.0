<?php
/**
 * Class field_permission_patterns_ui
 */


class field_permission_patterns_ui extends ctools_export_ui {

  /**
   * Field_permission_patterns edit form.
   */
  public function edit_form(&$form, &$form_state) {
    parent::edit_form($form, $form_state);
    unset($form['info']);

    $fields = array();
    foreach (field_info_fields() as $field) {
      $type = "fpp_{$form_state['item']->machine_name}";
      if (isset($field['field_permissions']['type']) && $field['field_permissions']['type'] == $type) {
        $fields[] = $field['field_name'];
      }
    }

    if (!empty($fields)) {
      $form['container'] = array(
        '#type' => 'fieldset',
        '#title' => t('Linked fields'),
        '#description' => t("The following fields are currently using this pattern. If you change access settings every linked field settings going to be updated."),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      );
      $form['container']['fields'] = array(
        '#markup' => theme('item_list', array('items' => $fields)),
      );
    }

    $form['label'] = array(
      '#type' => 'textfield',
      '#required' => TRUE,
      '#title' => t('Label'),
      '#maxlength' => 60,
      '#default_value' => $form_state['item']->label,
    );

    $form['machine_name'] = array(
      '#type' => 'machine_name',
      '#title' => t('Machine name'),
      '#maxlength' => 60,
      '#disabled' => FALSE,
      '#default_value' => $form_state['item']->machine_name,
      '#machine_name' => array(
        'exists' => '_field_permission_patterns_load_by_machine_name',
        'source' => array('label'),
      ),
    );

    $form['description'] = array(
      '#type' => 'textfield',
      '#title' => t('Description'),
      '#default_value' => $form_state['item']->description,
    );

    $form['field']['field_permissions'] = array(
      '#weight' => -10,
      '#access' => user_access('administer field permissions'),
    );

    $form['field_permissions']['#tree'] = TRUE;
    $form['field_permissions']['#access'] = user_access('administer field permissions');
    $form['field_permissions']['permissions'] = _field_permission_patterns_permission_matrix($form_state['item']);

    $form['#pre_render'][] = '_field_permissions_field_settings_form_pre_render';
  }

  /**
   * Field_permission_patterns edit form submit callback.
   */
  public function edit_form_submit(&$form, &$form_state) {
    parent::edit_form_submit($form, $form_state);

    $form_state['item']->settings = array();
    $permissions = $form_state['values']['field_permissions']['permissions']['checkboxes'];

    foreach ($permissions as $rid => $settings) {
      $form_state['item']->settings[$rid] = $settings;
      $form_state['item']->settings[$rid]['rid'] = $rid;
    }

    // Update every field where the currently edited pattern has used.
    foreach (field_info_fields() as $field) {
      if (isset($field['field_permissions']['type'])) {
        _field_permission_patterns_update_field_permissions($field, $field['field_name']);
      }
    }
  }

  /**
   * List options.
   */
  public function list_sort_options() {
    return array(
      'disabled' => t('Enabled, label'),
      'label' => t('label'),
    );
  }

  /**
   * Define the structure of a row.
   */
  public function list_build_row($item, &$form_state, $operations) {
    // Set up sorting
    switch ($form_state['values']['order']) {
      case 'disabled':
        $this->sorts[$item->label] = empty($item->disabled);
        break;

      case 'label':
        $this->sorts[$item->label] = $item->label;
        break;
    }

    $ops = theme('links__ctools_dropbutton', array('links' => $operations, 'attributes' => array('class' => array('links', 'inline'))));

    $this->rows[$item->label] = array(
      'data' => array(
        array('data' => check_plain($item->label), 'class' => array('ctools-export-ui-name')),
        array('data' => check_plain($item->description), 'class' => array('ctools-export-ui-name')),
        array('data' => $ops, 'class' => array('ctools-export-ui-operations')),
      ),
      'class' => array(!empty($item->disabled) ? 'ctools-export-ui-disabled' : 'ctools-export-ui-enabled'),
    );
  }

  /**
   * Define the listing header.
   */
  public function list_table_header() {
    return array(
      array('data' => t('Label'), 'class' => array('ctools-export-ui-label')),
      array('data' => t('Description'), 'class' => array('ctools-export-ui-description')),
      array('data' => t('Operations'), 'class' => array('ctools-export-ui-operations')),
   );
  }

}
