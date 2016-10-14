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

<div class="main-container container page inquerito-page">
    <div class="row">
        <section class="col-md-12">
            <a id="main-content"></a>
            <h1 class="page-header">Inquéritos</h1>
            <div class="region region-content">
                <div class="form-dropdown-menus clearfix">
                    <div class="section-icon">
                        <span class="icon-categorias"></span>
                    </div>
                    <div class="section-content">
                        <div class="form-select-element">
                            <label class="form-label label-category">Bairros</label>
                            <select id="bairros-form-select" name="category-form-select" class="form-control form-select">
                                <option value="">Escolher Bairro</option>
                            </select>
                        </div>
                        <div class="form-select-element">
                            <label class="form-label label-status">Rotas</label>
                            <select id="rotas-form-select" name="status-form-select" class="form-control form-select">
                                <option value="">Escolher Rota</option>
                            </select>
                        </div>
                        <div class="form-select-element">
                            <label class="form-label label-address">Mensagens</label>
                            <select id="messages-form-select" name="address-form-select" class="form-control form-select">
                                <option value="">Escolher Mensagem</option>
                            </select>
                        </div>
                    </div>
                    <div class="form-group">
                        <div class="col-sm-10" style="margin: 20px 0 20px 45px">
                            <button id="send-sms-button" type="button" class="btn btn-default btn-lg">
                                <span class="glyphicon glyphicon-send" aria-hidden="true"></span> Send
                            </button>
                        </div>
                    </div>
                </div>
            </div>
        </section>
        <section class="col-md-12" style="margin-top: 20px;">
            <div class="panel panel-default">
                <!-- Default panel contents -->
                <!-- <div class="panel-heading">Panel heading</div>
                <div class="panel-body">
                    <p>Some default panel content here. Nulla vitae elit libero, a pharetra augue. Aenean lacinia bibendum nulla sed consectetur. Aenean eu leo quam. Pellentesque ornare sem lacinia quam venenatis vestibulum. Nullam id dolor id nibh ultricies vehicula ut id elit.</p>
                </div> -->

                <!-- Table -->
                <table class="table">
                    <thead>
                        <tr>
                            <th>Bairro/Rota</th>
                            <th>Mensagem</th>
                            <th>Sim</th>
                            <th>Não</th>
                            <th>Data</th>
                        </tr>
                    </thead>
                    <tbody id="received-messages"></tbody>
                </table>
            </div>
        </section>
    </div>
</div>
<footer class="footer navbar-inverse navbar-fixed-bottom">
  <?php print render($page['footer']); ?>
</footer>