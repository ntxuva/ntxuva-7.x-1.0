(function($) {
    log = console.log;

    //set stats
    $.ajax({
        url: '/georeport/v2/requests.xml',
        success: function(res){
            var serviceNames = {},
                freqService  = {}, 
                countClosed  = 0,
                countTime    = 0,
                hours        = 0;

            $('request', res).each(function(index, request){
                var $status = $('status', request);

                if ($status.length && $status.text() == 'closed') {
                    var requested_datetime = $('requested_datetime', request).text(),
                        updated_datetime   = $('updated_datetime', request).text(),
                        reqDate = new Date(requested_datetime),
                        upDate  = new Date(updated_datetime);

                    if (typeof reqDate == 'object' && typeof upDate == 'object') {
                        countClosed++;
                        countTime += Math.abs(reqDate - upDate); 
                    }
                }

                var service_name = $('service_name', request).text();

                if (!serviceNames[service_name])
                    serviceNames[service_name] = 1;
                else
                    serviceNames[service_name] = serviceNames[service_name]+ 1;

                if (!freqService.name || freqService.count < serviceNames[service_name])
                    freqService = {'name': service_name, 'count': serviceNames[service_name]};
            });

            hours = Math.floor((countTime / countClosed) / 1000 / 60 / 60);
            $('.average-hours').text(hours);
            $('.frequent-request-label').text(freqService.name);
        }
    });
    
    $(document).ready(function(){
        $('.field-label').addClass('label');

        $('.geolocation-address-geocode, .geolocation-client-location, .geolocation-remove').addClass('btn');

        // Hide form input's address on focus.
        $('.geolocation-address input').focus(function(){
            this.value = '';
        });

        $('.node-report-form #edit-submit').html(Drupal.t('Next'));

        var currentHash = document.location.hash.replace(/^#/, '');
        if (currentHash) {
            $('.nav-tabs a[href=#' + currentHash + ']').tab('show');
        }

        $('.nav-tabs > li > a').on('click', function(e) {
            var linkHash = e.target.hash;

            $('html, body').animate(
                {
                    scrollTop: $(linkHash).offset().top
                }, 
                600, 
                function(){
                    window.location.hash = linkHash;
                }
            );

            if (linkHash.indexOf('3--') > -1 || linkHash.indexOf('---media') > -1)  {
                $('.node-report-form #edit-submit').html(Drupal.t('Save'));
            } 
            else {
                $('.node-report-form #edit-submit').html(Drupal.t('Next'));
            }
        });

        // Submit changes
        $('.node-report-form #edit-submit').click(function(e) {
            currentHash = document.location.hash.replace(/^#/, '');
            e.preventDefault();

            if (!currentHash || currentHash.indexOf('1--') > -1 || currentHash.indexOf('---location') > -1) {
                $('a:contains(2.)').tab('show');
                var hash = $('a:contains(2.)').attr('href');
                // animate
                $('html, body').animate({
                    scrollTop: $(hash).offset().top - 30 
                    }, 600, function(){
                    window.location.hash = hash;
                });
            } 
            else if (currentHash.indexOf('2--') > -1 || currentHash.indexOf('---your-report') > -1) {
                $('a:contains(3.)').tab('show');

                var hash = $('a:contains(3.)').attr('href');
                // animate
                $('html, body').animate({
                    scrollTop: $(hash).offset().top - 30
                    }, 600, function(){
                    window.location.hash = hash;
                });
                $('#edit-submit').html(Drupal.t('Save'));
            } 
            else if (currentHash.indexOf('3--') > -1 || currentHash.indexOf('---media') > -1) {
                $('form').unbind('submit').submit();
            }
        });

        //radio buttons
        var $categoriesContainer = $('.form-radios');

        function toggleCheck($radio) {
            var checked   = $radio.is(':checked'),
                $gChecked = [];
         
            if (checked) {
                $radio.parent().addClass('checked').siblings('.checked').removeClass('checked');
            }
            else {
                $radio.parent().removeClass('checked');
            }
        };

        $categoriesContainer.find('input.form-radio').on({
                change: function(e) {
                    toggleCheck($(this));
                },
                click: function(e) {
                    e.stopPropagation();
                },
                keyup: function() {
                    toggleCheck($(this));
                },
                focus: function(event) {
                    _self.$element.addClass('active');
                },
                blur: function(event) {                
                    _self.$element.removeClass('active');
                }
            }); 
    });
       
})(jQuery);
