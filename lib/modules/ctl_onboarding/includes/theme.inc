<?php

function theme_ctl_onboarding_table($variables) {
  $element     = $variables['element'];
  $showing_all = isset($element['#showing_all']) ? $element['#showing_all'] : FALSE;
  $header = array(
    t('Sunday'),
    t('Monday'),
    t('Tuesday'),
    t('Wednesday'),
    t('Thursday'),
    t('Friday'),
    t('Saturday'),
  );

  $days = array('su', 'm', 'tu', 'w', 'th', 'f', 'sa');
  $rows = array();

  // Setup the table.
  for ($hour = CTL_ONBOARDING_FIRST_START_HOUR; $hour <= CTL_ONBOARDING_LAST_START_HOUR; $hour += CTL_ONBOARDING_ONE_HOUR) {
    $row = array();
    for ($i = 0, $l = count($days); $i < $l; ++$i) {
      $slot_id = $days[$i] . '-' . $hour;
      $slot    = isset($element[$slot_id]) ? $element[$slot_id] : NULL;
      $row[]   = (isset($slot) && ($showing_all || $slot['#title'])) ? drupal_render($slot) : NULL;
    }
    // Only add rows if they are not completely empty.
    $r = array_filter($row, function ($x) { return isset($x); });
    if (!empty($r)) {
      $rows[$hour] = $row;
    }
  }

  $table = theme('table', array(
    'header' => $header,
    'rows'   => $rows,
  ));

  return $table;
}
