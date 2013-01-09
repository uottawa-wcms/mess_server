<?php

abstract class ServiceControllerBase {

  private $errored = FALSE;
  private $errorMessages = array();

  protected function error($message) {
    $this->errored = TRUE;
    $this->errorMessages[] = $message;
  }

  protected function clearErrors() {
    $this->errored = FALSE;
    $this->errorMessages = array();
  }

  public function checkErrors() {
    if ($this->errored) {
      return $this->errorMessages;
    }
    return FALSE;
  }

  public abstract function provide($settings);
  public abstract function settingsForm($form, &$form_state, $settings = array());
  public abstract function validateForm($form, &$form_state);
  public abstract function buildSettings($form, &$form_state);

}

abstract class CachedServiceController extends ServiceControllerBase {

  public function provide($settings) {
    // we hash the serialized value to make sure it's within the
    // max length of the field and to make sure all data types can
    // be returned from buildCacheIdentifier
    $cacheID = hash('sha256', serialize($this->buildCacheIdentifier($settings)));
    if ($this->checkCacheExpiry($cacheID, $settings)) {
      $this->refreshCache($cacheID, $settings);
    }
    return $this->retrieveCache($cacheID);
  }

  private function retrieveCache($cacheID) {
    $results = cache_get($cacheID, 'service_controller_cache');
    return $results['service_results'];
  }

  private function refreshCache($cacheID, $settings) {
    $results = array(
      'service_results' => $this->buildForCache($settings),
      'built' => time(),
    );
    cache_set($cacheID, $results, 'service_controller_cache');
  }

  private function checkCacheExpiry($cacheID, $settings) {
    $results = cache_get($cacheID, 'service_controller_cache');
    $age = time() - $results['built'];
    if ($age >= $settings['max_age']) {
      return TRUE;
    }
    return FALSE;
  }

  public function settingsForm($form, &$form_state, $settings = array()) {
    $form['cache_settings'] = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#title' => t('Cache Settings'),
      'max_age' => array(
        '#type' => 'textfield',
        '#title' => t('Cache Expiry'),
        '#description' => t('Number of seconds to keep results in cache.'),
        '#required' => TRUE,
        '#default_value' => isset($settings['max_age']) ? $settings['max_age'] : 0,
      ),
    );
    return $form + $this->extendSettingsForm($form, $form_state, $settings);
  }

  public function validateForm($form, &$form_state) {
    if (!filter_var($form_state['values']['max_age'], FILTER_VALIDATE_INT)) {
      form_set_error('max_age', t('You must enter an integer value for the cache'));
    }
    $this->extendValidateForm($form, $form_state);
  }

  public function buildSettings($form, &$form_state) {
    return array(
      'max_age' => $form_state['values']['max_age'],
    ) + $this->extendBuildSettings($form, $form_state);
  }

  protected abstract function extendSettingsForm($form, &$form_state, $settings = array());
  protected abstract function extendValidateForm($form, &$form_state);
  protected abstract function extendBuildSettings($form, &$form_state);

  protected abstract function buildCacheIdentifier($settings);

  /**
   * This replaces the standard provide() method for cached services.
   */
  protected abstract function buildForCache($settings);
}