<?php
/**
 * @file
 * Map page template for Mark-a-Spot
 */
?>

<div class="navbar-wrapper">
  <div class="navbar navbar-inverse navbar-static-top" role="navigation">
    <div class="container-fluid secondary">
      <div class="navbar-header">
        <?php if ($logo): ?>
          
        <?php endif; ?>

        <?php if (!empty($site_name)): ?>
          <a class="name navbar-brand secondary" href="<?php print $front_page; ?>" title="<?php print t('Home'); ?>"></a>
        <?php endif; ?>

        <!-- .btn-navbar is used as the toggle for collapsed navbar content -->
        <button type="button" class="navbar-toggle" data-toggle="collapse" data-target=".navbar-collapse">
          <span class="sr-only">Toggle navigation</span>
          <span class="icon-bar"></span>
          <span class="icon-bar"></span>
          <span class="icon-bar"></span>
        </button>
      </div>


      <?php if (!empty($primary_nav) || !empty($secondary_nav) || !empty($page['navigation'])): ?>
        <div class="collapse navbar-collapse">
          <nav class="nav navbar-nav">
            <?php if (!empty($primary_nav)): ?>
              <?php print render($primary_nav); ?>
            <?php endif; ?>
            <?php if (!empty($secondary_nav)): ?>
              <?php print render($secondary_nav); ?>
            <?php endif; ?>
            <?php if (!empty($page['navigation'])): ?>
              <?php print render($page['navigation']); ?>
            <?php endif; ?>
          </nav>
        </div>
      <?php endif; ?>
    </div>
  </div>
</div>
<div id="map" style="width:100%; height:100%"> </div>
<div class="main-container container">

  <header role="banner" id="page-header">
    <?php if (!empty($site_slogan)): ?>
      <p class="lead"><?php print $site_slogan; ?></p>
    <?php endif; ?>

    <?php print render($page['header']); ?>
  </header> <!-- /#header -->

  <div class="row map-tools">

    <?php if (!empty($page['sidebar_first'])): ?>
      <aside class="col-md-3" role="complementary">
        <?php print render($page['sidebar_first']); ?>
      </aside>  <!-- /#sidebar-first -->
    <?php endif; ?>

    <?php if (!empty($page['sidebar_second'])): ?>
      <aside class="" role="complementary">
        <?php print render($page['sidebar_second']); ?>
      </aside>  <!-- /#sidebar-second -->
    <?php endif; ?>
	  
       <?php if (!empty($page['highlighted'])): ?>
        <div class="highlighted hero-unit"><?php print render($page['highlighted']); ?></div>
      <?php endif; ?>
      
  </div>
</div>
<footer class="footer navbar-inverse navbar-fixed-bottom">
  <?php print render($page['footer']); ?>
</footer>
