<?php

function theme_view_mess_server($vars) {
  drupal_add_css(drupal_get_path('module', 'mess_server') . '/mess_server.css');
  $html = '';
  $html .= '<h2 class="service_title">' . check_plain($vars['title']) . '</h2>';
  $html .= '<p class="service_key">[' . check_plain($vars['service_key']) . ']</p>';
  $html .= '<p class="service_type">' . check_plain($vars['service_type']) . '</p>';
  $html .= '<div class="service_description">' . check_plain($vars['description']) . '</div>';
  $html .= '<p class="service_add_token">' . l('Add Token', 'admin/structure/remote_services/' . $vars['service_key'] . '/add_token') . '</p>';

  $header = array(
    'key' => t('Token'),
    'secret' => t('Secret'),
    'status' => t('Status'),
    'requests' => t('Requests'),
    'errors' => t('Errors'),
    'actions' => t('Actions'),
  );
  $rows = array();
  foreach ($vars['tokens'] as $token) {
    $actions = array(
      l('edit', 'admin/structure/remote_services/'.$vars['service_key'].'/'.$token['user_key'].'/edit'),
    );
    $rows[] = array(
      'key' => theme('html_tag', array(
        'element' => array(
          '#tag' => 'span',
          '#value' => $token['user_key'],
          '#attributes' => array(
            'title' => $token['usage_note'],
          ),
        )
      )),
      'secret' => $token['user_token'],
      'status' => $token['has_access'] == 'Y' ? t('Enabled') : t('Disabled'),
      'requests' => theme('html_tag', array(
        'element' => array(
          '#tag' => 'span',
          '#value' => $token['count'],
          '#attributes' => array(
            'title' => 'Last Access: ' . (empty($token['last']) ? t('Never') : format_date($token['last'], 'long')),
          ),
        ),
      )),
      'errors' => $token['errors'],
      'actions' => implode(' | ', $actions),
    );
  }
  $html .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'empty' => t('No tokens have been added yet. Please add one using the \'Add Token\' button.'),
  ));
  return $html;
}