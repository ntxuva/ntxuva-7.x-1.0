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

<div class="main-container container page relatorio-page">
    <div class="row">
        <section class="col-md-12">
            <a id="main-content"></a>
            <h1 class="page-header">Relatórios</h1>
            <div class="region region-content">
                <div class="form-dropdown-menus clearfix">
                    <div class="section-icon">
                        <span class="icon-categorias"></span>
                    </div>
                    <div class="section-content">
                        <div class="form-select-element">
                            <label class="form-label label-category">Categorias</label>
                            <select id="category-form-select" name="category-form-select" class="form-control form-select">
                                <option value="">Escolher Categoria</option>
                            </select>
                        </div>
                        <div class="form-select-element">
                            <label class="form-label label-status">Estados</label>
                            <select id="status-form-select" name="status-form-select" class="form-control form-select">
                                <option value="">Escolher Estados</option>
                            </select>
                        </div>
                        <div class="form-select-element">
                            <label class="form-label label-address">Bairros</label>
                            <select id="address-form-select" name="address-form-select" class="form-control form-select">
                                <option value="">Escolher Bairro</option>
                            </select>
                        </div>
                    </div>
                </div>
                
                <div class="section-pie-charts clearfix">
                    <div class="section-icon">
                        <span class="icon-pies"></span>
                    </div>
                    <div class="section-content">
                        <div id="service-chart" class="chart-container">
                            <canvas width="200" height="200"></canvas>
                            <label>Categorias</label>
                            <ul class="chart-legend"></ul>
                        </div>
                        <div id="status-chart" class="chart-container">
                            <canvas width="200" height="200"></canvas>
                            <label>Estados</label>
                            <ul class="chart-legend"></ul>
                        </div>
                        <div id="address-chart" class="chart-container">
                            <canvas width="200" height="200"></canvas>
                            <label>Bairros</label>
                            <ul class="chart-legend"></ul>
                        </div>
                    </div>
                </div>
                <div class="section-dates">
                    <div class="section-icon">
                        <span class="icon-time"></span>
                    </div>
                    <div class="section-content">
                        <label class="year-label">Escolher Ano</label>
                        <select id="year-form-select" name="year-form-select" class="form-control form-select"></select>                        
                        <ul class="months-list"></ul>
                        <label class="month-label">Escolher Mês</label>
                    </div>
                </div>
                <div class="section-line-chart">
                    <div class="section-icon">
                        <span class="icon-stats"></span>
                    </div>
                    <div class="section-content">
                        <canvas id="line-chart" width="900" height="400"></canvas>
                    </div>
                </div>
            </div>
        </section>
    </div>
</div>
<footer class="footer navbar-inverse navbar-fixed-bottom">
  <?php print render($page['footer']); ?>
</footer>
