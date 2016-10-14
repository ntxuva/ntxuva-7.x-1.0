<?php
/**
 * @file
 * Map page template for Mark-a-Spot
 */
 drupal_add_library('system', 'ui.datepicker');
drupal_add_js("(function ($) { $('.datepicker').datepicker(); })(jQuery);", array('type' => 'inline', 'scope' => 'footer', 'weight' => 5));
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



<script type="text/javascript">
(function ($) {
$(document).ready(function(e){
    $('.search-panel .dropdown-menu').find('a').click(function(e) {
		e.preventDefault();
		var param = $(this).attr("href").replace("#","");
		var concept = $(this).text();
		$('.search-panel span#search_concept').text(concept);
		$('.input-group #search_param').val(param);
	});
});
})(jQuery);

(function ($) {
$(document).ready(function(e){
    $('.search-panel1 .dropdown-menu').find('a').click(function(e) {
		e.preventDefault();
		var param = $(this).attr("href").replace("#","");
		var concept = $(this).text();
		$('.search-panel1 span#search_concept1').text(concept);
		$('.input-group #search_param1').val(param);
	});
});
})(jQuery);
</script>

<style type="text/css">
.page-header,
.view-filters{
display: none;
}
</style>
						
<div class="container searchbox_list">
    <div class="row">
    
        <div class="col-xs-12">
            <h1 class=" ">
                Processos
            </h1>
			<form method="get" action="/processos" autocomplete="off">  

		    <div class="input-group">
                <div class="input-group-btn search-panel">
                    <button type="button" class="btn btn-default dropdown-toggle" data-toggle="dropdown">
                    	<span id="search_concept">Tipo de Processo</span> <span class="caret"></span>
                    </button>
                    <ul class="dropdown-menu" role="menu">
                      <li><a href="#11">Contentor não recolhido</a></li>
                      <li><a href="#10">Contentor a Arder</a></li>
                      <li><a href="#12">Lixo fora do contentor</a></li>
                      <li><a href="#13">Tchova não passou</a></li>
                      <li><a href="#15">Lixeira informal</a></li>
                      <li class="divider"></li>
                      <li><a href="#All">Tudo</a></li>
                    </ul>
                </div>
                <input type="hidden" name="field_category_tid" value="All" id="search_param">
                <div class="input-group-btn search-panel1">
                    <button type="button" class="btn btn-default dropdown-toggle" data-toggle="dropdown">
                    	<span id="search_concept1">Estado do Processo</span> <span class="caret"></span>
                    </button>
                    <ul class="dropdown-menu" role="menu1">
                      <li><a href="#5">Registados</a></li>
                      <li><a href="#6">Em Processo</a></li>
                      <li><a href="#7">Resolvidos</a></li>
                      <li><a href="#8">Arquivados</a></li>
                      <li><a href="#74">Invalidos</a></li>
                      <li class="divider"></li>
                      <li><a href="#All">Tudo</a></li>
                    </ul>
                </div>
                <input type="hidden" name="field_status_tid" value="All" id="search_param1">   
                <input type="text" class="form-control" name="field_address_value" placeholder="Pesquise por Local, código postal ou endereço">
                <span class="input-group-btn">
                    <button class="btn btn-primary submit"><span class="glyphicon glyphicon-search"></span></button>
                </span>
            </div> 

<hr>
<div class="container">
<h3 id="linked-pickers">Datas</h3>
    <div class="col-md-6">
        <div class="form-group">
            <div class="input-group date" id="datetimepicker6">
                <input type="text" class="datepicker form-control" name="changed[min]" />
                <span class="input-group-addon"><span class="glyphicon glyphicon-calendar"></span>
                </span>
            </div>
        </div>
    </div>
    <div class="col-md-6">
        <div class="form-group">
            <div class="input-group date" id="datetimepicker7">
                <input type="text" class="datepicker form-control" name="changed[max]" />
                <span class="input-group-addon"><span class="glyphicon glyphicon-calendar"></span>
                </span>
            </div>
        </div>
    </div>
</div>
<hr>

			</form>
        </div>
	</div>
</div>










<div class="main-container container page">

  <div class="row">
    <?php if (!empty($page['sidebar_first'])): ?>
      <aside class="col-md-3" role="complementary">
        <?php print render($page['sidebar_first']); ?>
      </aside>  <!-- /#sidebar-first -->
    <?php endif; ?>

    <?php if (empty($page['sidebar_second'])): ?>
      <section class="col-md-12">
    <?php else:; ?>
      <section class="col-md-7">
    <?php endif; ?>
      <?php if (!empty($breadcrumb)): print $breadcrumb; endif;?>
      <a id="main-content"></a>
      <?php print render($title_prefix); ?>
      <?php if (!empty($title)): ?>
        <h1 class="page-header"><?php print $title; ?></h1>
      <?php endif; ?>
      <?php print render($title_suffix); ?>
      <?php print $messages; ?>
      <?php if (!empty($tabs)): ?>
        <?php print render($tabs); ?>
      <?php endif; ?>
      <?php if (!empty($page['help'])): ?>
        <div class="well secondary"><?php print render($page['help']); ?></div>
      <?php endif; ?>
      <?php if (!empty($action_links)): ?>
        <ul class="action-links"><?php print render($action_links); ?></ul>
      <?php endif; ?>
		        <?php if (!empty($page['highlighted'])): ?>
        <div class="highlighted hero-unit"><?php print render($page['highlighted']); ?></div>
      <?php endif; ?>
      <?php print render($page['content']); ?>
    </section>

    <?php if (!empty($page['sidebar_second'])): ?>
      <aside class="col-md-4 col-md-offset-1" role="complementary">
        <?php print render($page['sidebar_second']); ?>
      </aside>  <!-- /#sidebar-second -->
    <?php endif; ?>

  </div>
</div>

<footer class="footer navbar-inverse">
  <?php print render($page['footer']); ?>
</footer>
