<?php

/**
 * @file
 * Default template for HTML Mail
 *
 */
  $template_name = basename(__FILE__);
  $current_path = realpath(NULL);
  $current_len = strlen($current_path);
  $template_path = realpath(dirname(__FILE__));
  if (!strncmp($template_path, $current_path, $current_len)) {
    $template_path = substr($template_path, $current_len + 1);
  }
  $template_url = url($template_path, array('absolute' => TRUE));
?>
<div class="htmlmail-body">
<?php echo $body; ?>
</div>
<?php if ($debug):
  $module_template = "htmlmail--$module.tpl.php";
  $message_template = "htmlmail--$module--$key.tpl.php";
?>
<hr />
<div class="htmlmail-debug">
  <dl><dt><p>
    To customize this message:
  </p></dt><dd><ol><li><p><?php if (empty($theme)): ?>
    Visit <u>admin/config/system/htmlmail</u>
    and select a theme to hold your custom email template files.
  </p></li><li><p><?php elseif (empty($theme_path)): ?>
    Visit <u>admin/appearance</u>
    to enable your selected
    <u><?php echo drupal_ucfirst($theme); ?></u> theme.
  </p></li><li><?php endif;
if ("$template_path/$template_name" == "$theme_path/$message_template"): ?><p>
    Edit your<br />
    <code><?php echo "$template_path/$template_name"; ?></code>
    <br />file.
  </p></li><li><?php
else:
  if (!file_exists("$theme_path/htmlmail.tpl.php")): ?><p>
    Copy<br />
    <code><?php echo "$module_path/htmlmail.tpl.php"; ?></code>
    <br />to<br />
    <code><?php echo "$theme_path/htmlmail.tpl.php"; ?></code>
  </p></li><li><?php
  endif;
  if (!file_exists("$theme_path/$module_template")): ?><p>
    For module-specific customization, copy<br />
    <code><?php echo "$module_path/htmlmail.tpl.php"; ?></code>
    <br />to<br />
    <code><?php echo "$theme_path/$module_template"; ?></code>
  </p></li><li><?php
  endif;
  if (!file_exists("$theme_path/$message_template")): ?><p>
    For message-specific customization, copy<br />
    <code><?php echo "$module_path/htmlmail.tpl.php"; ?></code>
    <br />to<br />
    <code><?php echo "$theme_path/$message_template"; ?></code>
  </p></li><li><?php endif; ?><p>
    Edit the copied file.
  </p></li><li><?php
endif; ?><p>
    Send a test message to make sure your customizations worked.
  </p></li><li><p>
    If you think your customizations would be of use to others,
    please contribute your file as a feature request in the
    <a href="http://drupal.org/node/add/project-issue/htmlmail">issue queue</a>.
  </p></li></ol></dd><?php if (!empty($params)): ?><dt><p>
    The <?php echo $module; ?> module sets the <u><code>$params</code></u>
    variable.  For this message,
  </p></dt><dd><p><code><pre>
$params = <?php echo check_plain(print_r($params, 1)); ?>
  </pre></code></p></dd><?php endif; ?></dl>
</div>
<?php endif;
