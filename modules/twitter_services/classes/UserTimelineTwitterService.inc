<?php

class UserTimelineTwitterService extends CachedServiceController {

  protected function extendSettingsForm($form, &$state, $settings = array()) {
    $settings += array(
      'screen_name' => '',
      'count' => 3,
      'include_rts' => FALSE,
      'exclude_replies' => TRUE,
      'consumer_token' => '',
      'consumer_secret' => '',
      'user_token' => '',
      'user_secret' => '',
    );
    return array(
      'twitter_settings' => array(
        '#title' => t('Twitter Settings'),
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        'screen_name' => array(
          '#type' => 'textfield',
          '#required' => TRUE,
          '#title' => t('Twitter Username'),
          '#description' => t('Enter the username you would like to see tweets from'),
          '#default_value' => $settings['screen_name'],
        ),
        'count' => array(
          '#title' => t('Number of Tweets'),
          '#description' => t('Show up to this many tweets (this is not a guarentee that this many tweets will be present)'),
          '#type' => 'textfield',
          '#required' => TRUE,
          '#default_value' => $settings['count'],
        ),
        'include_rts' => array(
          '#title' => t('Include Retweets'),
          '#type' => 'checkbox',
          '#default_value' => $settings['include_rts'],
        ),
        'include_replies' => array(
          '#title' => t('Include Replies'),
          '#type' => 'checkbox',
          '#default_value' => !$settings['exclude_replies'],
        ),
      ),
      'twitter_oauth' => array(
        '#title' => t('Twitter OAuth Connection'),
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        'consumer_token' => array(
          '#type' => 'textfield',
          '#title' => t('Consumer Key'),
          '#required' => TRUE,
          '#default_value' => $settings['consumer_token'],
        ),
        'consumer_secret' => array(
          '#type' => 'textfield',
          '#title' => t('Consumer Secret'),
          '#required' => TRUE,
          '#default_value' => $settings['consumer_secret'],
        ),
        'user_token' => array(
          '#type' => 'textfield',
          '#title' => t('User Token'),
          '#required' => TRUE,
          '#default_value' => $settings['user_token'],
        ),
        'user_secret' => array(
          '#type' => 'textfield',
          '#title' => t('User Secret'),
          '#required' => TRUE,
          '#default_value' => $settings['user_secret'],
        ),
      ),
    );
  }

  protected function extendBuildSettings($form, &$form_state) {
    return array(
      'screen_name' => $form_state['values']['screen_name'],
      'count' => $form_state['values']['count'],
      'include_rts' => !!$form_state['values']['include_rts'],
      'exclude_replies' => !$form_state['values']['include_replies'],
      'consumer_token' => $form_state['values']['consumer_token'],
      'consumer_secret' => $form_state['values']['consumer_secret'],
      'user_token' => $form_state['values']['user_token'],
      'user_secret' => $form_state['values']['user_secret'],
    );
  }

  protected function extendValidateForm($form, &$form_state) {
    if (!filter_var($form_state['values']['count'], FILTER_VALIDATE_INT)) {
      form_set_error('count', t('You must enter an integer for the number of tweets'));
    }
  }

  protected function buildCacheIdentifier($settings) {
    return array(
      $settings['screen_name'],
      $settings['count'],
      $settings['include_rts'],
      $settings['exclude_replies'],
    );
  }

  protected function buildForCache($settings) {
    $service = '1.1/statuses/user_timeline';
    $connection = twitter_services_get_connection(
        $settings['consumer_token'],
        $settings['consumer_secret'],
        $settings['user_token'],
        $settings['user_secret']
    );
    $params = array(
      'screen_name' => $settings['screen_name'],
      'count' => $settings['count'],
      'include_rts' => $settings['include_rts'] ? 'true' : 'false',
      'exclude_replies' => $settings['exclude_replies'] ? 'true' : 'false',
    );
    return twitter_services_rest_call($connection, $service, $params, 'GET');
  }

}
