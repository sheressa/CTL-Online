<?php

$path = DRUPAL_ROOT . '/' . drupal_get_path('module', 'ctl_onboarding');
require_once "$path/includes/background-check.inc";
require_once "$path/includes/helper.inc";
require_once "$path/includes/slot.inc";


/**
 * Implements hook_form().
 */
function ctl_onboarding_form($form, &$form_state) {
  $pages_text = variable_get('ctl_onboarding_pages_text');

  if (isset($form_state['storage'])) {
    $storage = $form_state['storage'];
    if (!empty($storage['affirm_registration'])) {
      $register_text   = isset($pages_text['register']) ? $pages_text['register'] : NULL;
      $success_message = isset($register_text['success_message']) ? token_replace($register_text['success_message']) : NULL;
      $form['registration_successful'] = array(
        '#type'   => 'markup',
        '#markup' => $success_message,
      );
      return $form;
    }
    if (!empty($storage['confirm_defer'])) {
      $defer_text  = isset($pages_text['defer']) ? $pages_text['defer'] : NULL;
      $question    = isset($defer_text['confirmation_title'])
                   ? token_replace($defer_text['confirmation_title'])
                   : 'Are you sure you want to defer?';
      $description = isset($defer_text['confirmation_message'])
                   ? token_replace($defer_text['confirmation_message'])
                   : 'If you defer you can come back and sign up for training at a later date.';
      $path        = 'registration';
      $yes         = 'Yes, I wish to defer';
      $no          = 'No, I do not wish to defer';
      $name        = 'defer';
      return confirm_form($form, $question, $path, $description, $yes, $no, $name);
    }
    if (!empty($storage['confirm_decline'])) {
      // $question    = '';
      $decline_text = isset($pages_text['decline']) ? $pages_text['decline'] : NULL;
      $question     = isset($decline_text['confirmation_title'])
                    ? token_replace($decline_text['confirmation_title'])
                    : 'Are you sure you want to decline?';
      $description  = isset($decline_text['confirmation_message'])
                    ? token_replace($decline_text['confirmation_message'])
                    : 'If you decline you will not be able to sign up for training later.';
      $path         = 'registration';
      $yes          = 'Yes, I wish to decline';
      $no           = 'No, I do not wish to decline';
      $name         = 'decline';
      return confirm_form($form, $question, $path, $description, $yes, $no, $name);
    }
  }

  // TODO later ?
  // - add visual indicators for statuses

  $register_text = isset($pages_text['register']) ? $pages_text['register'] : NULL;

  $registration_period = intval(variable_get('ctl_onboarding_registration_period', CTL_ONBOARDING_REGISTRATION_CLOSED));
  if ($registration_period === CTL_ONBOARDING_REGISTRATION_CLOSED) {
    $registration_closed_text = isset($register_text['closed_message']) ? token_replace($register_text['closed_message']) : '';
    $form['registration_closed'] = array(
      '#type'   => 'markup',
      '#markup' => $registration_closed_text,
    );
    return $form;
  }

  $registration_open_text = isset($register_text['open_message']) ? token_replace($register_text['open_message']) : '';

  $form['registration'] = array(
    '#type'   => 'markup',
    '#markup' => $registration_open_text,
  );

  $slot_trainers = variable_get('ctl_onboarding_slot_trainers');
  $slots = array();
  foreach ($slot_trainers as $slot_code => $v) {
    if ($v > 0) {
      $times               = explode('-', $slot_code);
      $start_hour_display  = _ctl_onboarding_format_hour_display($times[1]);
      $finish_hour_display = _ctl_onboarding_format_hour_display($times[1] + CTL_ONBOARDING_SESSION_DURATION);
      $slots[$slot_code]   = "$start_hour_display - $finish_hour_display ET";
    }
  }

  foreach ($slots as $slot_code => $slot) {
    $slot_status = _ctl_onboarding_slot_status($slot_code);
    if (!$slot_status['available']) {
      unset($slots[$slot_code]);
    }
  }

  $form['training_slot'] = array(
    '#type'    => 'radios',
    '#options' => $slots,
    '#theme'   => array('ctl_onboarding_table'),
  );

  $form['register'] = array(
    '#type'     => 'submit',
    '#value'    => 'Register',
    '#validate' => array('_ctl_onboarding_form_validate_register'),
    '#submit'   => array('_ctl_onboarding_form_submit_register'),
  );
  $form['defer'] = array(
    '#type'   => 'submit',
    '#value'  => 'Defer',
    '#submit' => array('_ctl_onboarding_form_submit_defer'),
  );
  $form['decline'] = array(
    '#type'   => 'submit',
    '#value'  => 'Decline',
    '#submit' => array('_ctl_onboarding_form_submit_decline'),
  );

  $form['#attached']['css'] = array(
    drupal_get_path('module', 'ctl_onboarding') . '/ctl_onboarding.css',
  );

  return $form;
}


// Used by the "defer" and "decline" confirmation forms.
function ctl_onboarding_form_submit($form, &$form_state) {
  $values    = $form_state['values'];
  $deferring = !empty($values['defer']);
  $declining = !empty($values['decline']);

  if ($deferring) {
    global $user;

    // Make sure we have custom profile fields by reloading the user explicitly.
    $user = user_load($user->uid);

    _ctl_onboarding_set_user_role($user->uid, CTL_ONBOARDING_ROLE_DEFERRED);

    drupal_set_message(t('You have chosen to defer.'));

    $mail_data = array(
      'key'  => 'defer',
      'user' => $user,
    );

    // Send the confirmation email for deferral.
    if (_ctl_onboarding_mail_send($mail_data)) {
      drupal_set_message(t('You should receive a confirmation email shortly.'));
    }
    else {
      drupal_set_message(t('There was a problem sending a confirmation email.'), 'error');
    }

    drupal_goto('training-exit-form');
  }
  elseif ($declining) {
    global $user;

    // Make sure we have custom profile fields by reloading the user explicitly.
    $user = user_load($user->uid);

    _ctl_onboarding_set_user_role($user->uid, CTL_ONBOARDING_ROLE_DECLINED);

    drupal_set_message(t('You have chosen to decline.'));

    $mail_data = array(
      'key'  => 'decline',
      'user' => $user,
    );

    // Send the confirmation email for declining.
    if (_ctl_onboarding_mail_send($mail_data)) {
      drupal_set_message(t('You should receive a confirmation email shortly.'));
    }
    else {
      drupal_set_message(t('There was a problem sending a confirmation email.'), 'error');
    }

    drupal_goto('training-exit-form');
  }
}


// TODO later ?
function _ctl_onboarding_form_validate_register($form, &$form_state) {
  // drupal_set_message(t("hi"));

  // $values = $form_state['values'];
  // $unmet_requirements = array(_ctl_graduation_courses_completed($values['uid']))
  //                     + $values['additional_requirements'];
  // $unmet_requirements = array_filter($unmet_requirements, function ($v) { return empty($v); });
  // if (!empty($unmet_requirements)) {
  //   form_set_error('additional_requirements', t('The trainee must pass all the requirements.'));
  // }
}


function _ctl_onboarding_form_submit_register($form, &$form_state) {
  $values    = $form_state['values'];
  $slot_code = $values['training_slot'];

  // Only allow registrations from qualified users.
  $role = user_role_load_by_name(CTL_ONBOARDING_ROLE_DEFERRED);
  if (user_has_role($role->rid)) {
    drupal_set_message(t('You have already deferred so you cannot register.'), 'error');
    return;
  }
  $role = user_role_load_by_name(CTL_ONBOARDING_ROLE_DECLINED);
  if (user_has_role($role->rid)) {
    drupal_set_message(t('You have already declined so you cannot register.'), 'error');
    return;
  }

  global $user;

  $registered = _ctl_onboarding_slot_register($slot_code, $user->uid);
  if ($registered) {
    $form_state['rebuild'] = TRUE;
    $form_state['storage']['affirm_registration'] = TRUE;
  }
}


function _ctl_onboarding_form_submit_defer($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['confirm_defer'] = TRUE;
}


function _ctl_onboarding_form_submit_decline($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['confirm_decline'] = TRUE;
}
