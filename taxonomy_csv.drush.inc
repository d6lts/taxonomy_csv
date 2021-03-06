<?php

/**
 * @file
 * Drush commands for taxonomy CSV import/export.
 */

/**
 * Implements hook_drush_command().
 */
function taxonomy_csv_drush_command() {
  $items = array();

  $items['taxocsv-import'] = array(
    'aliases' => array('vocimp'),
    'callback' => 'drush_taxonomy_csv_import',
    'description' => 'Import taxonomies and hierarchical structures with CSV file.',
    'examples' => array(
      'drush taxocsv-import my_file alone_terms' => 'Import my_file using the default settings.',
      'drush taxocsv-import my_file tree_structure --keep_order --delimiter=";" --check_line --vocabulary_target=existing --vocabulary_id=3 --existing_items=update_replace --result_terms' => 'Import my_file as a tree structure into vocabulary with vid 2, a semicolon for delimiter and default settings for other options, and display imported terms after process.',
    ),
    'arguments' => array(
      'file_path'     => 'Required. The full path or url to CSV file to import',
      'import_format' => "Optional. CSV format: 'alone_terms' (default), 'def_links', 'flat', 'tree_structure', 'polyhierarchy', 'parents', 'children', 'relations', 'synonyms', 'definitions', 'descriptions', 'weights' or 'fields'",
    ),
    'options' => array(
      'custom_format' => "List of comma separated fields (default: 'name') [>=D7 only]",
      'keep_order'    => 'Keep order of imported terms',
      'delimiter'     => 'CSV delimiter (default: ",")',
      'enclosure'     => "CSV enclosure (default: none or '\"')",
      'check_line'    => 'Check format of lines',
      'check_utf8'    => 'Check utf8 format',
      'locale_custom' => 'Specific locale of imported file',
      'vocabulary_target' => "'autocreate' (default), 'duplicate' or 'existing' (default if a vocabulary_id is set)",
      'vocabulary_id' => 'vid or machine_name (>=D7 only) of the vocabulary to duplicate or to import into',
      'custom_fields' => 'Custom comma separated fields to add or create [>=D7 only]',
      'delete_terms'  => 'Delete all terms before import',
      'check_hierarchy' => 'Check vocabulary hierarchy',
      'set_hierarchy' => "If hierarchy isn't checked, set it (0, 1 or 2 (default))",
      'existing_items' => "What to do with existing items, which depends on import format:\n'update' (default), 'replace' or 'ignore'",
      // Specific to relations import:
      'relations_create_subrelations' => 'Create subrelations when importing relations',
      'relations_all_vocabularies'   => 'Allows relations with terms in other vocabularies',
      // Level of result process infos.
      'result_stats' => 'Display import stats',
      'result_terms' => 'Display list of imported terms',
      'result_level' => "Level of displayed messages: 'first' (default), 'warnings', 'notices' or 'infos'",
      'result_type'  => "Group infos 'by_message' (default) or 'by_line'",
    ),
  );

  $items['taxocsv-export'] = array(
    'aliases' => array('vocexp'),
    'callback' => 'drush_taxonomy_csv_export',
    'description' => 'Export terms and properties to a CSV file.',
    'examples' => array(
      'drush taxocsv-export 2 def_links' => "Export vocabulary with vid 2 with 'Full definition and links' format, using the default settings.",
      'drush taxocsv-export 2 tree_structure --delimiter=";" --order=name' => 'Export vocabulary with vid 2 as a CSV tree_structure ordered by name, with a semicolon for delimiter, and default settings for other options.',
    ),
    'arguments' => array(
      'vocabulary_id' => 'Required. vid or machine_name (>=D7 only) of the vocabulary to export (0 means all)',
      'export_format' => "Optional. CSV format: 'alone_terms' (default), 'tid_name', 'tree_structure', 'def_links', 'fields', 'parents', 'children', 'relations', 'synonyms', 'definitions', 'descriptions' or 'weights'",
      'file_path'     => "Optional. Full path or filename of exported csv file (default: [current directory]/taxocsv.csv)",
    ),
    'options' => array(
      'custom_format' => "List of comma separated fields (default: 'name') [>=D7 only]",
      'delimiter'     => 'One character csv delimiter (default: ",")',
      'enclosure'     => "Zero or one character csv enclosure (default: '\"')",
      'line_ending'   => "'Unix' (default), 'Mac' or 'MS-DOS'",
      'order'         => "Order of terms: 'name' (default), 'tid' or 'weight'",
      // Specific to def_links:
      'def_links_terms_ids'        => "'name_if_needed' (default), 'name' or 'tid'",
      'def_links_vocabularies_ids' => "'none' (default), 'name' or 'vid'",
      // Level of result process infos.
      'result_duplicates' => "Display duplicate terms",
    ),
  );

  return $items;
}

/**
 * Implements hook_drush_help().
 */
function taxonomy_csv_drush_help($section) {
  switch ($section) {
    case 'drush:taxocsv-import':
      return dt('Import taxonomies and hierarchical structures with CSV file.');
    case 'drush:taxocsv-export':
      return dt('Export terms and properties to a CSV file.');
  }
}

/**
 * Process the import of an input.
 *
 * @see drush_invoke()
 * @see drush.api.php
 */
function drush_taxonomy_csv_import($file_path, $import_format = 'alone_terms') {
  // Start process.
  drush_print('Checking options...');

  if (!$file_path || !file_exists($file_path)) {
    drush_log('You need to set the correct path or url of the file to import.', 'error');
    return;
  }

  require_once(drupal_get_path('module', 'taxonomy_csv') . '/import/taxonomy_csv.import.api.inc');

  // Set arguments.
  $options = array();
  $options['url'] = $file_path;
  $options['import_format'] = $import_format;

  // Get the defaults options and update them with user ones.
  $options += _taxonomy_csv_values('import_default_api');

  // Set simple options.
  foreach (array(
      'delimiter',
      'enclosure',
      'locale_custom',
      'vocabulary_target',
      'vocabulary_id',
      'set_hierarchy',
      'existing_items',
      // Level of result process infos.
      'result_level',
      'result_type',
    ) as $value) {
    $option = drush_get_option($value);
    if ($option !== NULL) {
      $options[$value] = $option;
    }
  }

  // If user don't use 'vocabulary_target' but only 'vocabulary_id', this
  // vocabulary will be used. See https://drupal.org/node/1475952.
  if ((drush_get_option('vocabulary_target') === NULL) && (drush_get_option('vocabulary_id') !== NULL)) {
    $options['vocabulary_target'] = 'existing';
  };

  // Set boolean options.
  foreach (array(
      'keep_order',
      'check_line',
      'check_utf8',
      'delete_terms',
      'check_hierarchy',
      'set_hierarchy',
      // Specific to relations import:
      'relations_create_subrelations',
      'relations_all_vocabularies',
      // Level of result process infos.
      'result_stats',
      'result_terms',
    ) as $value) {
    $options[$value] = (drush_get_option($value) === NULL) ? FALSE : TRUE;
  }

  // Set array options.
  foreach (array(
      'custom_format',
      'custom_fields',
    ) as $value) {
    $option = drush_get_option_list($value);
    if ($option !== array()) {
      $options[$value] = $option;
    }
  }

  // Set general options.
  $options['check_options'] = TRUE;
  $options['result_display'] = TRUE;

  // Prepare import.
  $messages = taxonomy_csv_import($options);

  if (count($messages)) {
    foreach ($messages as $message) {
      $msg = str_replace('<br />', " \n", $message);
      drush_log('- ' . $msg, 'error');
    }
    return;
  }

  // Process the batch.
  drush_log('Options are good.', 'status');
  drush_print('Launch import process. Please wait...');
  $batch =& batch_get();
  $batch['progressive'] = FALSE;
  drush_backend_batch_process();

  // End of drush process.
  drush_print('End of drush import batch.');
}

/**
 * Process the export of a vocabulary.
 *
 * @see drush_invoke()
 * @see drush.api.php
 */
function drush_taxonomy_csv_export($vocabulary_id, $export_format = 'alone_terms', $file_path = '') {
  // Start process.
  drush_print('Checking options...');

  if ($vocabulary_id === NULL) {
    drush_log('You need to set a vocabulary id (0 means all).', 'error');
    return;
  }

  // Set the destination file path.
  if ($file_path == '') {
    $file_path = getcwd() . '/taxocsv.csv';
  }

  require_once(drupal_get_path('module', 'taxonomy_csv') . '/export/taxonomy_csv.export.api.inc');

  // Set arguments.
  $options = array();
  $options['vocabulary_id'] = $vocabulary_id;
  $options['export_format'] = $export_format;

  // Get the defaults options and update them with user ones.
  $options += _taxonomy_csv_values('export_default_api');

  // Set simple options.
  foreach (array(
      'delimiter',
      'enclosure',
      'line_ending',
      'order',
      // Specific to def_links:
      'def_links_terms_ids',
      'def_links_vocabularies_ids',
      // Level of result process infos.
    ) as $value) {
    $option = drush_get_option($value);
    if ($option !== NULL) {
      $options[$value] = $option;
    }
  }

  // Set boolean options.
  foreach (array(
      'result_duplicates',
    ) as $value) {
    $options[$value] = (drush_get_option($value) === NULL) ? FALSE : TRUE;
  }

  // Set array options.
  foreach (array(
    ) as $value) {
    $option = drush_get_option_list($value);
    if ($option !== array()) {
      $options[$value] = $option;
    }
  }

  // Set general options.
  $options['check_options'] = TRUE;
  $options['result_display'] = TRUE;

  // Prepare export.
  $messages = taxonomy_csv_export($options);
  $file = '';
  if (is_object($messages)) {
    $file = $messages;
    $messages = array();
  }

  if (count($messages)) {
    foreach ($messages as $message) {
      $msg = str_replace('<br />', " \n", $message);
      drush_log('- ' . $msg, 'error');
    }
    return;
  }

  // Process the batch.
  drush_log('Options are good.', 'status');
  drush_print('Launch export process. Please wait...');
  $batch =& batch_get();
  $batch['progressive'] = FALSE;
  drush_backend_batch_process();

  // Move file to chosen directory if needed.
  if (file_exists($file->filepath)) {
    rename($file->filepath, $file_path);
    drush_log("The vocabulary has been exported to $file_path.", 'status');
  }

  // End of drush process.
  drush_print('End of drush export batch.');
}
