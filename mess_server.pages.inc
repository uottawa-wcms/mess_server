<?php

function mess_server_provider($service) {
  $token = $_POST['service_token'];
  $secret = $_POST['service_secret'];
  $token_object = mess_server_token_load($service->key, $token);
  $valid = FALSE;
  if ($token_object) {
    if ($token_object->user_token == $secret) {
      $valid = TRUE;
    }
  }
  if (!$valid) {
    $result_object = mess_server_generate_error('Invalid token for service');
  } else {
    $result_object = mess_server_execute($service, $token_object);
  }
  print(json_encode($result_object));
  exit();
}

function mess_server_list() {
  $headers = array(
    'title' => t('Title'),
    'key' => t('Access Key'),
    'actions' => t('Actions'),
  );
  $rows = array();
  $rs = db_query('SELECT * FROM {mess_server_services} ORDER BY title ASC');
  foreach ($rs as $row) {
    $actions = array();
    $actions[] = l('view', 'admin/structure/remote_services/' . $row->service_key);
    $actions[] = l('edit', 'admin/structure/remote_services/' . $row->service_key . '/edit');
    $rows[] = array(
      'title' => $row->title,
      'key' => $row->service_key,
      'actions' => implode(' | ', $actions),
    );
  }
  return theme('table', array(
    'header' => $headers,
    'rows' => $rows,
  ));
}

function mess_server_service_form($form, &$form_state, $service = NULL) {
  $controller = NULL;
  $controller_key = NULL;
  $settings = array();
  $form['is_new'] = array(
    '#type' => 'value',
    '#value' => isset($form_state['values']['is_new']) ? $form_state['values']['is_new'] :
        (empty($service) ? TRUE : FALSE),
  );
  if (empty($form_state['step'])) {
    if (empty($service)) {
      $form_state['step'] = 1;
    } else {
      $form_state['step'] = 2;
      $controller = mess_server_get_controller($service->definition);
      $controller_key = $service->definition;
      $settings = $service->settings;
    }
  } else {
    $controller = mess_server_get_controller($form_state['values']['service_def']);
    $controller_key = $form_state['values']['service_def'];
  }
  if ($form_state['step'] == 1) {
    $controllers = mess_server_get_controllers();
    $options = array();
    foreach ($controllers as $key => $controller) {
      $options[$key] = $controller['category'] . ' - ' . $controller['title'];
    }
    $form['service_def'] = array(
      '#title' => t('Service'),
      '#description' => t('Select the service you would like to use'),
      '#required' => 'true',
      '#type' => 'select',
      '#options' => $options,
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Next'),
      '#submit' => array('mess_server_service_form_submit'),
      '#limit_validation_errors' => array(array('service_def')),
    );
    return $form;
  } else {
    $form['service_def'] = array(
      '#type' => 'value',
      '#value' => $controller_key,
    );
    $form['service_info'] = array(
      '#type' => 'fieldset',
      '#title' => t('Service Information'),
      '#collapsible' => TRUE,
      'title' => array(
        '#type' => 'textfield',
        '#title' => t('Title'),
        '#maxlength' => 255,
        '#required' => TRUE,
        '#description' => t('Administrative title for the service'),
        '#default_value' => !empty($service) ? $service->title : '',
      ),
      'key' => array(
        '#type' => 'machine_name',
        '#title' => t('Access Key'),
        '#maxlength' => 64,
        '#description' => t('Key which will be used to access this service'),
        '#default_value' => !empty($service) ? $service->service_key : '',
        '#access' => empty($service),
        '#machine_name' => array(
          'exists' => 'mess_server_service_key_exists',
          'source' => array('title'),
        ),
      ),
      'description' => array(
        '#type' => 'textarea',
        '#title' => t('Usage'),
        '#description' => t('Describe how this service is meant to be used'),
        '#default_value' => !empty($service) ? $service->description : '',
      ),
    );
    $settings = empty($service) ? array() : $service->settings;
    $form += $controller->settingsForm($form, $form_state, $settings);
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}
function mess_server_service_form_validate($form, &$form_state) {
  if ($form_state['step'] == 2) {
    $controller = mess_server_get_controller($form_state['values']['service_def']);
    $controller->validateForm($form, $form_state);
  }
}

function mess_server_service_form_submit($form, &$form_state) {
  if ($form_state['step'] == 1) {
    $form_state['step'] = 2;
    $form_state['rebuild'] = TRUE;
  } else {
    $controller = mess_server_get_controller($form_state['values']['service_def']);
    $settings = $controller->buildSettings($form, $form_state);
    $object = (object) array(
      'title' => $form_state['values']['title'],
      'description' => $form_state['values']['description'],
      'service_key' => $form_state['values']['key'],
      'definition' => $form_state['values']['service_def'],
      'settings' => $settings,
    );
    if (empty($form['is_new'])) {
      drupal_write_record('mess_server_services', $object);
      drupal_set_message(t('Managed external service added'));
    } else {
      drupal_write_record('mess_server_services', $object, array('service_key'));
      drupal_set_message(t('Managed external service updated'));
    }
    $form_state['redirect'] = 'admin/structure/remote_services/' . $object->service_key;
  }
}

function mess_server_service_info($service) {
  $vars = array();
  $vars += (array) $service;
  $vars['controller'] = mess_server_get_controller($service->definition);
  $vars['service_definition'] = mess_server_get_controllers($service->definition);
  $vars['service_type'] = $vars['service_definition']['title'];
  $vars['tokens'] = array();
  $rs = db_query('SELECT * FROM {mess_server_access} WHERE service_key = :key', array(
    ':key' => $service->service_key
  ));
  foreach ($rs as $row) {
    $vars['tokens'][] = (array) $row;
  }
  return theme('view_mess_server', $vars);
}

function mess_server_token_form($form, &$form_state, $service, $token = NULL) {
  if (empty($token)) {
    $token = (object) array(
      'service_key' => $service->service_key,
      'user_key' => '',
      'user_token' => md5(uniqid('', TRUE)),
      'has_access' => 'Y',
      'usage_note' => '',
      'is_new' => TRUE,
    );
  }
  $form += array(
    'is_new' => array(
      '#type' => 'value',
      '#value' => isset($token->is_new) ? $token->is_new : FALSE,
    ),
    'service_key' => array(
      '#type' => 'value',
      '#value' => $token->service_key,
    ),
    'user_key' => array(
      '#type' => 'textfield',
      '#title' => t('User Key'),
      '#description' => t('This will uniquely identify a user of the service'),
      '#required' => TRUE,
      '#maxlength' => 64,
      '#default_value' => $token->user_key,
      '#access' => !empty($token->is_new),
    ),
    'user_token' => array(
      '#type' => 'textfield',
      '#title' => t('User Secret'),
      '#description' => t('Copy this and keep it a secret! Only give it to the user of the service.'),
      '#required' => TRUE,
      '#maxlength' => 64,
      '#default_value' => $token->user_token,
    ),
    'has_access' => array(
      '#type' => 'checkbox',
      '#title' => t('Enabled'),
      '#default_value' => $token->has_access == 'Y',
    ),
    'usage_note' => array(
      '#type' => 'textarea',
      '#title' => t('Usage'),
      '#default_value' => $token->usage_note,
      '#description' => t('Describe how this token will be used'),
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => 'Save',
    )
  );
  return $form;
}

function mess_server_token_form_validate($form, &$form_state) {
  if ($form_state['values']['is_new']) {
    $token = mess_server_token_load(
        $form_state['values']['service_key'],
        $form_state['values']['user_key']);
    if ($token) {
      form_set_error('user_key', t('This key is already in use for this service'));
    }
  }
}

function mess_server_token_form_submit($form, &$form_state) {
  $token = (object) array(
    'service_key' => $form_state['values']['service_key'],
    'user_key' => $form_state['values']['user_key'],
    'user_token' => $form_state['values']['user_token'],
    'has_access' => $form_state['values']['has_access'] ? 'Y' : 'N',
    'usage_note' => $form_state['values']['usage_note'],
  );
  if ($form_state['values']['is_new']) {
    drupal_write_record('mess_server_access', $token);
  } else {
    drupal_write_record('mess_server_access', $token, array('service_key', 'user_key'));
  }
  $form_state['redirect'] = 'admin/structure/remote_services/' . $form_state['values']['service_key'];
  drupal_set_message(t('Token saved'));
}