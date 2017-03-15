<?php
/**
 * Drush commands for importing initial pages, news and forums to the Drupal
 * site at http://nordicmicroalgae.org.
 *
 * Copyright (c) 2017 SMHI, Swedish Meteorological and Hydrological Institute
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

function nordicmicroalgae_drush_command() {
  $items['nordicmicroalgae-import-forums'] = array(
    'description' => 'Import forums from YAML document',
    'arguments' => array(
      'path' => 'Path to YAML document with forums',
    ),
    'required-arguments' => TRUE,
  );
  $items['nordicmicroalgae-import-news'] = array(
    'description' => 'Import news from YAML document.',
    'arguments' => array(
      'path' => 'Path to YAML document with news',
    ),
    'required-arguments' => TRUE,
    'callback' => 'drush_nordicmicroalgae_import_nodes',
    'callback arguments' => array('news', 'News'),
  );
  $items['nordicmicroalgae-import-pages'] = array(
    'description' => 'Import pages from YAML document.',
    'arguments' => array(
      'path' => 'Path to YAML document with pages',
    ),
    'required-arguments' => TRUE,
    'callback' => 'drush_nordicmicroalgae_import_nodes',
    'callback arguments' => array('page', 'Pages'),
  );
  $items['nordicmicroalgae-import-users'] = array(
    'description' => 'Import users from CSV document',
    'arguments' => array(
      'path' => 'Path to CSV document with users'
    ),
    'required-arguments' => TRUE,
  );
  return $items;
}

function drush_nordicmicroalgae_import_nodes($node_type, $root_key, $path) {
  $data = yaml_parse_file($path);

  if ($data === FALSE) {
      return drush_log(sprintf('Failed to parse YAML (%s)', $path), 'error');
  }

  if (empty($data[$root_key])) {
    return drush_log(sprintf("'%s' does not exist in '%s'", $root_key, $path), 'error');
  }

  $items = $data[$root_key];

  foreach ($items as $item) {

    if (empty($item['Title'])) {
      drush_log('Missing title in entry. Moving on to next', 'warning');
      continue;
    }

    // Setup common node properties.
    $node = (object) array(
      'type'     => $node_type,
      'language' => LANGUAGE_NONE,
      'title'    => $item['Title'],
    );
    node_object_prepare($node);

    // Setup author and date properties.
    if (!empty($item['Author'])) {
        $node->name = $item['Author'];
    }
    if (!empty($item['Submitted'])) {
      $node->date = $item['Submitted'];
    }
    node_submit($node);

    // Setup body field - convert markdown to html.
    if (!empty($item['Content'])) {
      $body_html = _drush_nordicmicroalgae_markdown_to_html($item['Content']);
    }
    else {
      $body_html = '';
    }
    $node->body[$node->language][0]['value'] = $body_html;
    $node->body[$node->language][0]['summary'] = '';
    $node->body[$node->language][0]['format'] = 'full_html';

    // Setup path alias - if specified.
    if (!empty($item['Path'])) {
      $node->path['alias'] = $item['Path'];
      $node->path['language'] = $node->language;
    }

    // Save node.
    node_save($node);
  }

}

function drush_nordicmicroalgae_import_forums($path) {
  $data = yaml_parse_file($path);

  if ($data === FALSE) {
      return drush_log(sprintf('Failed to parse YAML (%s)', $path), 'error');
  }

  if (empty($data['Forums'])) {
    return drush_log(sprintf("'Forums' does not exist in '%s'", $path), 'error');
  }

  $forum_sort_order = 0;

  $forum_vocabulary_id = variable_get('forum_nav_vocabulary', '');

  // Create a taxonomy term for each forum (forum title and description).
  foreach ($data['Forums'] as $forum_info) {

    // Setup term properties.
    $term = (object) array(
      'name'        => $forum_info['Title'],
      'description' => $forum_info['Description'],
      'parent'      => array(0),
      'weight'      => $forum_sort_order++,
      'vid'         => $forum_vocabulary_id,
    );

    // Save taxonomy term.
    taxonomy_term_save($term);

    // Create a node for each topic in this forum.
    foreach ($forum_info['Topics'] as $topic_info) {

      // Setup common node properties.
      $node = (object) array(
        'type'     => 'forum',
        'language' => LANGUAGE_NONE,
        'title'    => $topic_info['Title'],
      );
      node_object_prepare($node);

      // Setup author and date properties.
      if (!empty($topic_info['Author'])) {
          $node->name = $topic_info['Author'];
      }
      if (!empty($topic_info['Submitted'])) {
        $node->date = $topic_info['Submitted'];
      }
      node_submit($node);

      // Setup body field - convert markdown to html.
      if (!empty($topic_info['Content'])) {
        $body_html = _drush_nordicmicroalgae_markdown_to_html($topic_info['Content']);
      }
      else {
        $body_html = '';
      }
      $node->body[$node->language][0]['value'] = $body_html;
      $node->body[$node->language][0]['summary'] = '';
      $node->body[$node->language][0]['format'] = 'full_html';

      // Add reference to the taxnoomy term (forum) previously created.
      $node->taxonomy_forums[$node->language][0]['tid'] = $term->tid;

      // Save node.
      node_save($node);

      // Continue to the next topic if current topic does not have any comments/replies.
      if (empty($topic_info['Comments'])) {
        continue;
      }

      // Otherwise, create a comment node for each reply.
      foreach ($topic_info['Comments'] as $comment_info) {

        // Setup common comment properties.
        $comment = (object) array(
          'node_type' => 'comment_node_forum',
          'nid'       => $node->nid, // Reference to previously created node (topic).
          'name'      => '',
          'mail'      => '',
          'homepage'  => '',
          'subject'   => $comment_info['Title'],
          'comment'   => '',
          'cid'       => NULL,
          'pid'       => NULL,
          'language'  => LANGUAGE_NONE,
          'uid'       => 0,
          'status'    => COMMENT_PUBLISHED,
        );

        // Setup author and date properties.
        if (!empty($comment_info['Author'])) {
          $comment->name = $comment_info['Author'];
        }
        if (!empty($comment_info['Submitted'])) {
          $comment->date = $comment_info['Submitted'];
        }
        comment_submit($comment);

        // Override 'changed' (set by comment_submit).
        $comment->changed = $comment->created;

        // Setup body field - convert markdown to html.
        if (!empty($comment_info['Content'])) {
          $comment_body_html = _drush_nordicmicroalgae_markdown_to_html($comment_info['Content']);
        }
        else {
          $comment_body_html = '';
        }
        $comment->comment_body[$comment->language][0]['value']  = $comment_body_html;
        $comment->comment_body[$comment->language][0]['format'] = 'full_html';

        // Save comment.
        comment_save($comment);
      }
    }
  }
}

function drush_nordicmicroalgae_import_users($path) {

  // Read CSV to array.
  $rows = array_map('str_getcsv', file($path));

  // Mapping from CSV to Drupal fields (default fields).
  $user_default_fields = array(
    'Username'     => 'name',
    'E-mail'       => 'mail',
    'Roles'        => 'roles',
    'Created date' => 'created',
    'Last access'  => 'access',
  );

  // Mapping from CSV to Drupal fields (custom fields).
  $user_custom_fields = array(
    'Name'                 => 'field_user_name',
    'Institute'            => 'field_user_institute',
    'Address'              => 'field_user_address',
    'Postal code'          => 'field_user_postal_code',
    'City'                 => 'field_user_city',
    'Country'              => 'field_user_country',
    'Phone'                => 'field_user_phone',
    'Mobile'               => 'field_user_mobile',
    'Member of HELCOM-PEG' => 'field_user_member_of_helcom_peg',
    'Member of NOMP'       => 'field_user_member_of_nomp',
  );

  // Member of ... fields. Used to convert to Yes/No to 1/0.
  $user_member_of_fields = array(
    'helcom_peg',
    'nomp',
  );

  // Mapping from user role name to user role id.
  $user_role_name_to_id = array();

  foreach ($rows as $row_index => $row) {
    // First row should be headers.
    if ($row_index == 0) continue;

    $new_user = array();

    foreach ($row as $column_index => $column_value) {
      $key   = trim($rows[0][$column_index]);
      $value = trim($column_value);

      // Lookup if this is a default or custom field.
      if (isset($user_default_fields[$key])) {
        $new_user[$user_default_fields[$key]] = $value;
      }
      else if (isset($user_custom_fields[$key])) {
        $new_user[$user_custom_fields[$key]][LANGUAGE_NONE][0]['value'] = $value;
      }
      else {
        drush_log(sprintf("Unknown field '%s.'", $key), 'warning');
      }
    }

    // Convert user role names to ids.
    if (isset($new_user['roles'])) {
      $roles = array_map('trim', explode(',', $new_user['roles']));

      $new_user['roles'] = array();

      foreach ($roles as $role) {
        if (!isset($user_role_name_to_id[$role])) {
          $user_role_name_to_id[$role] = user_role_load_by_name($role);
        }

        $role_obj = $user_role_name_to_id[$role];

        if ($role_obj === FALSE) {
          drush_log(sprintf("Unknown user role '%s'.", $role), 'warning');
          continue;
        }

        $new_user['roles'][$role_obj->rid] = $role_obj->rid;
      }
    }

    // Convert date strings to timestamps.
    if (!empty($new_user['created'])) {
      $new_user['created'] = strtotime($new_user['created']);
    }
    if (!empty($new_user['access'])) {
      $new_user['access'] = strtotime($new_user['access']);
    }

    // Convert Yes/No to 1/0 for member of.. fields
    foreach ($user_member_of_fields as $member_field) {
      $is_member = &$new_user["field_user_member_of_$member_field"][LANGUAGE_NONE][0]['value'];
      $is_member = $is_member == 'Yes' ? 1 : 0;
    }

    // Set a random password.
    $new_user['pass'] = user_password(32);

    // User should be considered active.
    $new_user['status'] = 1;

    // Save user to database.
    $user = user_save(NULL, $new_user, NULL);

    if ($user) {
      drush_log(sprintf("Created user '%s' with uid '%u'.", $user->name, $user->uid), 'success');
    }
    else {
      drush_log(sprintf("Could not create user '%s'.", $new_user['name']), 'warning');
    }
  }

}

function _drush_nordicmicroalgae_markdown_to_html($source) {
  require_once 'Michelf/MarkdownExtra.inc.php';
  return Michelf\MarkdownExtra::defaultTransform($source);
}
