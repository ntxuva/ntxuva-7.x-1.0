if (typeof console.log != 'undefined')    
    log = console.log;

if (!Array.prototype.indexOf) {
    Array.prototype.indexOf = function (searchElement, fromIndex) {
        if ( this === undefined || this === null ) {
            throw new TypeError( '"this" is null or not defined' );
        }

        var length = this.length >>> 0; // Hack to convert object.length to a UInt32

        fromIndex = +fromIndex || 0;

        if (Math.abs(fromIndex) === Infinity) {
            fromIndex = 0;
        }

        if (fromIndex < 0) {
            fromIndex += length;
            if (fromIndex < 0) {
                fromIndex = 0;
            }
        }

        for (;fromIndex < length; fromIndex++) {
            if (this[fromIndex] === searchElement) {
            return fromIndex;
            }
        }

        return -1;
    };
}

Date.prototype.getMonthWeek = function(){
    var firstDay = new Date(this.getFullYear(), this.getMonth(), 1).getDay();
    return Math.ceil((this.getDate() + firstDay)/7);
};

// Chart.defaults.global.animation = false;

var lang_pt = {
    "months": ["Janeiro", "Fevereiro", "Março", "Abril", "Maio", "Junho", "Julho", "Agosto", "Setembro", "Outubro", "Novembro", "Dezembro"]
};

(function($) {
    var date= new Date();
    date.setDate(date.getDate()-180); // simplest estimate of 6 months
    var defaultStartDate = date.getFullYear()+'-'+date.getMonth()+'-'+date.getDate();
    var InqueriesReceive = {
        inited: false,
        replies: [],
        getReplies: function(){
            var _self = this;

            $.ajax({
                // url: 'replies-url',
                url: 'http://demo.ntxuva.org:5000/survey',
                method: 'GET',
                success: function(res){
                    _self.inited = true;
                    _self.replies = $.parseJSON(res) || res;
                    _self.parseReplies();
                }
            });
        },
        parseReplies: function(){
            var _self = this;

            if (!_self.inited)
                return;

            var repliesMarkup = [];

            $.each(_self.replies, function(index, reply){
                repliesMarkup.push('<tr>\
                                    <td scope="row">' + reply.area + '</td>\
                                    <td>' + unescape(reply.message) + '</td>\
                                    <td>' + reply.count_yes + '</td>\
                                    <td>' + reply.count_no + '</td>\
                                    <td>' + reply.date + '</td>\
                                </tr>');
            });

            this.$receivedMessages.append(repliesMarkup.join(''));
        },
        hooks: function(){
            var _self = this;

            this.$receivedMessages = $('#received-messages');
        },
        init: function(){
            var _self = this;

            if (!$('.inquerito-page').length)
                return;

            this.hooks();
            this.getReplies();
        }
    };
    
    var InqueriesSent = {
        inited: false,
        messages: [],
        districts: {},
        getMessages: function(){
            var _self = this;

            $.ajax({
                url: '/profiles/markaspot/themes/ntxuva/js/messages.json',
                //url: 'messages_shade.json',
                success: function(res){
                    _self.messages = $.parseJSON(res) || res;
                    _self.getNeighbourhoods();
                }
            });
        },
        getNeighbourhoods: function(){
            var _self = this;

            $.ajax({
                url: '/profiles/markaspot/themes/ntxuva/js/neighbourhoods.json',
                //url: 'neighbourhoods_shade.json',
                success: function(res){
                    _self.hooks();
                    _self.inited = true;
                    _self.districts = $.parseJSON(res) || res;
                    _self.parseDistricts();
                    _self.parseMessages();
                }
            });
        },
        parseMessages: function(clear){
            var _self = this;

            if (!_self.inited)
                return;

            var messagesMarkup = ['<option selected>Select message</option>'];

            if (!clear) {
                $.each(_self.messages, function(index, message){
                    messagesMarkup.push('<option value="' + encodeURIComponent(message) + '">' + message + '</option>');
                });
            }

            this.$messagesSelect.html(messagesMarkup.join(''));
        },
        parseDistricts: function(){
            var _self = this;

            if (!_self.inited)
                return;

            var districtMarkup = ['<option selected>Select district</option>'];
            console.log(typeof _self.districts);
            console.log(_self.districts);
            $.each(_self.districts, function(index, district){
                districtMarkup.push('<option value="' + district.id + '">' + district.name + '</option>');
            });

            this.$districtSelect.html(districtMarkup.join(''));
        },
        parseNeighbourhoods: function(){
            var _self = this;

            if (!_self.inited)
                return;

            var district = _self.$districtSelect.val(),
                neighbourhoodMarkup = ['<option selected>Select neighbourhood</option>'];

            if (!district || !_self.districts[district-1])
                return;

            $.each(_self.districts[district-1].neighbourhoods, function(index, neighbourhood){
                neighbourhoodMarkup.push('<option value="' + neighbourhood.id + '">' + neighbourhood.name + '</option>');
            });

            this.$neighbourhoodSelect.html(neighbourhoodMarkup.join(''));
        },
        parsePoints: function(){
            var _self = this;

            if (!_self.inited)
                return;

            var district = _self.$districtSelect.val(),
                neighbourhood = _self.$neighbourhoodSelect.val(),
                pointsMarkup = ['<option selected>Select point</option>'];

            if (!district || !neighbourhood || !_self.districts[district-1].neighbourhoods[neighbourhood-1]){
                return;
            }

            $.each(_self.districts[district-1].neighbourhoods[neighbourhood-1].points, function(index, point){
                pointsMarkup.push('<option value="' + point.id + '">' + point.name + '</option>');
            });

            this.$pointsSelect.html(pointsMarkup.join(''));
        },
        parsePhones: function(){
            var _self = this;
            
            if (!_self.inited)
                return;

            var district = _self.$districtSelect.val(),
                neighbourhood = _self.$neighbourhoodSelect.val(),
                point        = _self.$pointsSelect.val(),
                phones       = [];

            if (!district || !neighbourhood || !point){
                console.log("No district, neighbourhood or point, ", "district: ", district, ", neighbourhoods: ", neighbourhood, ", point:", point);
                return;
            }
            
            if (!_self.districts[district-1].neighbourhoods[neighbourhood-1] || !_self.districts[district-1].neighbourhoods[neighbourhood-1].points[point-1]){
                console.log("Neighborhood", _self.districts[district-1].neighbourhoods[neighbourhood-1]);
                console.log("Point", _self.districts[district-1].neighbourhoods[neighbourhood-1].points[point-1]);
                return;
            }

            $.each(_self.districts[district-1].neighbourhoods[neighbourhood-1].points[point-1].monitors, function(index, monitor){
                _self.sendSms(monitor.phone);
            });
        },
        sendSms: function(phoneNum){
            var _self   = this,
                message = this.$messagesSelect.val();

            if (!phoneNum || !message){
                alert("no phoneNum or message. phoneNum: ", phoneNum, ", message: ", message);
                return;
            }


            var district = _self.$districtSelect.val(),
                neighbourhood = _self.$neighbourhoodSelect.val(),
                point        = _self.$pointsSelect.val();

            var _neighbourhood = _self.districts[district-1].neighbourhoods[neighbourhood-1].name,
                _point = _self.districts[district-1].neighbourhoods[neighbourhood-1].points[point].name;

            $.ajax({
                // url: 'http://wasp.sourcecode.solutions:8080/MessagingGW/sendsms?from=mopa&to=' + phoneNum + '&text=' + message,
                url: 'http://demo.ntxuva.org:5000/survey',
                type: 'POST',
                data: {
                    'neighbourhood': _neighbourhood,
                    'quarter': _point,
                    'question_id': message,
                    'question' : message,
                    'to': phoneNum
                },
                success: function(res){
                    alert('Message send to ' + phoneNum);
                },
                error: function(xhr, textStatus, thrownError){
                    alert("Error sending message ", textStatus);
                }
            });
        },
        hooks: function(){
            var _self = this;
            this.$districtSelect = $('#districtos-form-select');
            this.$neighbourhoodSelect    = $('#bairros-form-select');
            this.$pointsSelect   = $('#rotas-form-select');
            this.$messagesSelect = $('#messages-form-select');
            this.$sendSms        = $('#send-sms-button');

            this.$districtSelect.on({
                change: function(event){
                    _self.parseNeighbourhoods();
                    _self.parsePoints();
                }
            });
            this.$neighbourhoodSelect.on({
                change: function(event){
                    _self.parsePoints();
                    // _self.parseMessages(true);
                }
            });

            this.$pointsSelect.on({
                change: function(event){
                    // _self.parseMessages();
                }
            });

            this.$sendSms.on({
                click: function(){
                    _self.parsePhones();
                }
            });
        },
        init: function(){
            var _self = this;

            if (!$('.inquerito-page').length)
                return;

            this.getMessages();
        }
    };

    var HomePageStats = {
        parse: function(){
            var serviceNames  = {},
                locationNames = {},
                freqService   = {},
                freqLocation  = {},
                countClosed   = 0,
                countTime     = 0,
                hours         = 0;

            $('request', this.res).each(function(index, request){
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

                if (service_name) {
                    if (!serviceNames[service_name])
                        serviceNames[service_name] = 1;
                    else
                        serviceNames[service_name] = serviceNames[service_name] + 1;

                    if (!freqService.name || freqService.count < serviceNames[service_name])
                        freqService = {'name': service_name, 'count': serviceNames[service_name]};
                }

                var location_name = $('neighbourhood', request).text().split(',')[0];

                if (location_name) {
                    if (!locationNames[location_name])
                        locationNames[location_name] = 1;
                    else
                        locationNames[location_name] = locationNames[location_name] + 1;

                    if (!freqLocation.name || freqLocation.count < locationNames[location_name])
                        freqLocation = {'name': location_name, 'count': locationNames[location_name]};
                }
            });

            hours = Math.floor((countTime / countClosed) / 1000 / 60 / 60);
            $('.average-hours').text(hours);
            $('.frequent-request-label').text(freqService.name);
            $('.frequent-location-label').text(freqLocation.name);
        },
        init: function(){
            var _self = this;
            if (!$('body').hasClass('front'))
                return;

            $.ajax({
                url: '/georeport/v2/requests.json?start_date='+defaultStartDate, //default to load requets for the last 6 months
                success: function(res){
                    _self.res = res;
                    _self.parse();
                }
            });
        }
    };

    var ChartsPage = {
        inited: false,
        chartDataMapping: {
            'service': {},
            'status' : {},
            'address': {}
        },
        chartsColors: {
            'service': {},
            'status' : {},
            'address': {}
        },
        line_chart: {},
        dates: {},
        parse: function(){
            var _self = this,

                serviceMarkup = [],
                statusMarkup  = [],
                addressMarkup = [],

                _serviceData  = (_self.chartDataMapping.service = {}),
                _statusData   = (_self.chartDataMapping.status  = {}),
                _addressData  = (_self.chartDataMapping.address = {}),

                serviceSelected = this.$categorySelect.val(),
                statusSelected  = this.$statusSelect.val(),
                addressSelected = this.$addressSelect.val();

            this.line_chart = {};
            this.dates      = {};

            window.a=this.res;
            this.res.forEach(function(request, index){
                var service_name = request.service_name,
                    status_name  = request.service_notice,
                    address_name = request.neighbourhood,
                    req_datetime = request.requested_datetime,

                    serviceNameEncode = encodeURIComponent(service_name),
                    statusNameEncode  = encodeURIComponent(status_name),
                    addressNameEncode = encodeURIComponent(address_name),

                    colorBase;

                if (service_name) {
                    if ((!serviceSelected || serviceSelected == serviceNameEncode) && (!statusSelected || statusSelected == statusNameEncode) && (!addressSelected || addressSelected == addressNameEncode)) {
                        if (!_serviceData[serviceNameEncode]) {
                            colorBase = _self.chartsColors.service[serviceNameEncode] || (_self.chartsColors.service[serviceNameEncode] = 'rgba(' + (Math.floor(Math.random() * 256)) + ',' + (Math.floor(Math.random() * 256)) + ',' + (Math.floor(Math.random() * 256)));

                            _serviceData[serviceNameEncode] = {
                                'label': service_name,
                                'value': 1,
                                'color': colorBase + ', 0.7)',
                                'highlight': colorBase + ', 1)'
                            };

                            serviceMarkup.push('<option value="' + serviceNameEncode + '">' + service_name + '</option>');
                        }
                        else {
                            _serviceData[serviceNameEncode].value = _serviceData[serviceNameEncode].value + 1;
                        }
                    }
                }

                if (status_name) {
                    if ((!statusSelected || statusSelected == statusNameEncode) && (!serviceSelected || serviceSelected == serviceNameEncode) && (!addressSelected || addressSelected == addressNameEncode)) {
                        if (!_statusData[statusNameEncode]) {
                            colorBase = _self.chartsColors.status[statusNameEncode] || (_self.chartsColors.status[statusNameEncode] = 'rgba(' + (Math.floor(Math.random() * 256)) + ',' + (Math.floor(Math.random() * 256)) + ',' + (Math.floor(Math.random() * 256)));

                            _statusData[statusNameEncode] = {
                                'label': status_name,
                                'value': 1,
                                'color': colorBase + ', 0.7)',
                                'highlight': colorBase + ', 1)'
                            };

                            statusMarkup.push('<option value="' + statusNameEncode + '">' + status_name + '</option>');
                        }
                        else {
                            _statusData[statusNameEncode].value = _statusData[statusNameEncode].value + 1;
                        }
                    }
                }

                if (address_name) {
                    if ((!addressSelected) && (!serviceSelected || serviceSelected == serviceNameEncode) && (!serviceSelected || serviceSelected == serviceNameEncode)) {
                        if (!_addressData[addressNameEncode]) {
                            colorBase = _self.chartsColors.address[addressNameEncode] || (_self.chartsColors.address[addressNameEncode] = 'rgba(' + (Math.floor(Math.random() * 256)) + ',' + (Math.floor(Math.random() * 256)) + ',' + (Math.floor(Math.random() * 256)));

                            _addressData[addressNameEncode] = {
                                'label': address_name,
                                'value': 1,
                                'color': colorBase + ', 0.7)',
                                'highlight': colorBase + ', 1)'
                            };

                            addressMarkup.push('<option value="' + addressNameEncode + '">' + address_name + '</option>');
                        }
                        else {
                            _addressData[addressNameEncode].value = _addressData[addressNameEncode].value + 1;
                        }
                    }
                }

                if (req_datetime) {
                    var req_date  = new Date(req_datetime),
                        req_year  = req_date.getFullYear(),
                        req_month = req_date.getMonth();
//update this to include years for not loaded requests
                    if (!_self.dates[req_year]) {
                        _self.dates[req_year] = [];   
                        for (var i = 0, l = lang_pt.months.length; i < l; i++) {
                            _self.dates[req_year].push({name: lang_pt.months[i], active: false});
                        }
                    }

                    _self.dates[req_year][req_month].active = true;

                    if (!_self.line_chart[req_year])
                        _self.line_chart[req_year] = [];

                    if (!_self.line_chart[req_year][req_month])
                        _self.line_chart[req_year][req_month] = [];

                    _self.line_chart[req_year][req_month].push(request);
                }
            });

            if (!this.inited) {
                this.$categorySelect.append(serviceMarkup.join(''));
                this.$statusSelect.append(statusMarkup.join(''));
                this.$addressSelect.append(addressMarkup.join(''));

                this.generateDates();
            }

            this.renderGraphs();

            if (serviceSelected || statusSelected || addressSelected) {
                this.renderLineGraph();            
            }
        },
        renderLineGraph: function(){
            var _self         = this,
                datasets      = {},
                yearSelected  = this.$yearSelect.val(),
                activeMonth   = this.$monthsList.find('.active:eq(0)'),
                lineGraphData = {
                    'labels': lang_pt.months,
                    'datasets': []
                },

                serviceSelected = this.$categorySelect.val(),
                statusSelected  = this.$statusSelect.val(),
                addressSelected = this.$addressSelect.val(),

                serviceName, statusName, serviceNameEncode, statusNameEncode, requestedDate;

            if (this.timelineChart) {
                this.timelineChart.destroy();
            }

            _data = this.line_chart[yearSelected];

            if (serviceSelected || addressSelected) {
                $.each(this.chartDataMapping.status, function(index, status){
                    var colorBase = _self.chartsColors.status[index];

                    datasets[index] = {
                        label: status.label,
                        fillColor: colorBase + ', 0.2)',
                        strokeColor: colorBase + ', 1)',
                        pointColor: colorBase + ', 1)',
                        pointStrokeColor: "#fff",
                        pointHighlightFill: "#fff",
                        pointHighlightStroke: colorBase + ', 1)',
                        data: []
                    };
                });
            }

            if (statusSelected || addressSelected) {
                $.each(this.chartDataMapping.service, function(index, service){
                    var colorBase = _self.chartsColors.service[index];

                    datasets[index] = {
                        label: service.label,
                        fillColor: colorBase + ', 0.2)',
                        strokeColor: colorBase + ', 1)',
                        pointColor: colorBase + ', 1)',
                        pointStrokeColor: "#fff",
                        pointHighlightFill: "#fff",
                        pointHighlightStroke: colorBase + ', 1)',
                        data: []
                    };
                });
            }

            if (statusSelected && serviceSelected) {
                $.each(this.chartDataMapping.address, function(index, address){
                    var colorBase = _self.chartsColors.address[index];

                    datasets[index] = {
                        label: address.label,
                        fillColor: colorBase + ', 0.2)',
                        strokeColor: colorBase + ', 1)',
                        pointColor: colorBase + ', 1)',
                        pointStrokeColor: "#fff",
                        pointHighlightFill: "#fff",
                        pointHighlightStroke: colorBase + ', 1)',
                        data: []
                    };
                });
            }

            if (activeMonth.length) {
                var b=activeMonth.data('id');
                _data = _data[activeMonth.data('id')];
                lineGraphData.labels = ['Semana 1', 'Semana 2', 'Semana 3', 'Semana 4', 'Semana 5'];
                //loop for each request for the selected month
                for (var i = 0, l = _data.length, c; i < l; i++) {
                    c = _data[i];
                    requestedDate = new Date(c.requested_datetime);
                    parseRequestData(c, requestedDate.getMonthWeek() - 1);
                }
            }
            else {
                //loop throught each month
                for (var i = 1, l = _data.length, c; i < l; i++) {
                    c = _data[i];
                    if (!c)
                        continue;

                    //loop for each request for this month
                    for (var j = 0, l2 = c.length; j < l2; j++) {
                        parseRequestData(c[j], i);
                    }
                }
            }

            function parseRequestData(reqData, arrayIndex){
                serviceName = reqData.service_name;
                statusName  = reqData.service_notice;
                addressName = reqData.neighbourhood;

                serviceNameEncode = encodeURIComponent(serviceName);
                statusNameEncode  = encodeURIComponent(statusName);
                addressNameEncode = encodeURIComponent(addressName);
                
                if (datasets[serviceNameEncode]) {
                    if (!datasets[serviceNameEncode].data[arrayIndex])
                        datasets[serviceNameEncode].data[arrayIndex] = 1;
                    else
                        datasets[serviceNameEncode].data[arrayIndex]++;
                }

                if (datasets[statusNameEncode]) {
                    if (!datasets[statusNameEncode].data[arrayIndex])
                        datasets[statusNameEncode].data[arrayIndex] = 1;
                    else
                        datasets[statusNameEncode].data[arrayIndex]++;
                }

                if (datasets[addressNameEncode]) {
                    if (!datasets[addressNameEncode].data[arrayIndex])
                        datasets[addressNameEncode].data[arrayIndex] = 1;
                    else
                        datasets[addressNameEncode].data[arrayIndex]++;
                }
            }

            lineGraphData.datasets = $.map(datasets, function(dataset){
                if (activeMonth.length) {
                    for (var i = 0, l = 5; i < l; i++) {
                        if (!dataset.data[i])
                            dataset.data[i] = 0;
                    }
                }
                else {
                    for (var i = 0, l = 12; i < l; i++) {
                        if (!dataset.data[i])
                            dataset.data[i] = 0;
                    }
                }
                
                return dataset;
            });

            if (!lineGraphData.datasets.length)
                return;
            
            this.timelineChart = new Chart(this.$timelineChart.get(0).getContext("2d")).Line(lineGraphData, {
                                    bezierCurve: false,
                                    multiTooltipTemplate: "<%if (datasetLabel){%><%=datasetLabel%>: <%}%><%= value %>"
                                });
        },
        renderGraphs: function(){
            var _self = this;

            this.servicePieChart.clear();
            this.statusPieChart.clear();
            this.addressPieChart.clear();

            this.servicePieChart.segments = [];
            this.statusPieChart.segments = [];
            this.addressPieChart.segments = [];

            //service chart
            $.each(this.chartDataMapping.service, function(i, c){
                _self.servicePieChart.addData(c);
            });
            this.servicePieChart.update();
            this.$serviceChart.find('.chart-legend:eq(0)').html(this.generateLegend(this.chartDataMapping.service).join(''));

            //status chart
            $.each(this.chartDataMapping.status, function(i, c){
                _self.statusPieChart.addData(c);
            });
            this.statusPieChart.update();
            this.$statusChart.find('.chart-legend:eq(0)').html(this.generateLegend(this.chartDataMapping.status).join(''));
            
            //address chart
            $.each(this.chartDataMapping.address, function(i, c){
                _self.addressPieChart.addData(c);
            });
            this.addressPieChart.update();
            this.$addressChart.find('.chart-legend:eq(0)').html(this.generateLegend(this.chartDataMapping.address).join(''));
        },
        generateLegend: function(data){
            return $.map(data, function(c){
                return '<li style="color:' + c.color + '">' + c.label + '</li>';
            });
        },
        generateDates: function(){
            var selectedYear = this.$yearSelect.val(),
                yearsDD      = [],
                monthsList   = [];

            for (var i in this.dates) {
                yearsDD.push('<option value="' + i + '">' + i + '</option>');
                if (!selectedYear)
                    selectedYear = i;
            }

            var _year = this.dates[selectedYear];

            for (var i = 0, l = _year.length; i < l; i++) {
                monthsList.push('<li class=' + (_year[i].active ? ('"enabled" data-id="' + i + '"') : "") + '>' + _year[i].name + '</li>');
            }

            if (!this.inited) {
                this.$yearSelect.append(yearsDD.join(''));
            }
            //clear the list first
            this.$monthsList.empty();
            this.$monthsList.append(monthsList.join(''));
        },
        hooks: function(){
            var _self = this;

            this.$serviceChart  = $('#service-chart');
            this.$statusChart   = $('#status-chart');
            this.$addressChart  = $('#address-chart');
            this.$timelineChart = $('#line-chart');
            this.$yearSelect    = $('#year-form-select');
            this.$monthsList    = $('.months-list');
            this.$categorySelect = $('#category-form-select');
            this.$statusSelect   = $('#status-form-select');
            this.$addressSelect  = $('#address-form-select');

            this.servicePieChart = new Chart(this.$serviceChart.find('canvas').get(0).getContext("2d")).Pie([], {tooltipTemplate: "<%= value %>", onShowTooltip: onShowTooltip});
            this.statusPieChart  = new Chart(this.$statusChart.find('canvas').get(0).getContext("2d")).Pie([], {tooltipTemplate: "<%= value %>", onShowTooltip: onShowTooltip});
            this.addressPieChart = new Chart(this.$addressChart.find('canvas').get(0).getContext("2d")).Pie([], {tooltipTemplate: "<%= value %>", onShowTooltip: onShowTooltip});

            this.$categorySelect.on({
                change: function(event){
                    _self.parse();
                }
            });

            console.log('yo');
            this.$yearSelect.on({
                change: function(event){
                    _self.generateDates();
                }
            })
            this.$statusSelect.on({
                change: function(event){
                    _self.parse();
                }
            });

            this.$addressSelect.on({
                change: function(event){
                    _self.parse();
                }
            });

            this.$monthsList.on({
                click: function(event){
                    var $eventTrigger = $(event.target);

                    if ($eventTrigger.hasClass('enabled')) {
                        if ($eventTrigger.hasClass('active')) {
                            $eventTrigger.removeClass('active');
                        }
                        else {
                            $eventTrigger.addClass('active');
                            $eventTrigger.siblings('.active').removeClass('active');
                        }

                        _self.parse();
                    }
                }
            });

            function onShowTooltip(segment){
                $legend = $('.section-pie-charts:eq(0)').find("li.highlighted");
                if ($legend.length) {
                    color = $legend.css('color');
                    $legend.css('color', color.replace(/\d+\.\d+\)$/, '0.7)')).removeClass('highlighted');
                }
                if (segment.length && segment[0].label) {
                    var $legend = $('.section-pie-charts:eq(0)').find("li:contains('" + segment[0].label + "')");
                    if (!$legend.length)
                        return;

                    var color = $legend.css('color');
                    $legend.css('color', color.replace(/\d+\.\d+\)$/, '0.99)')).addClass('highlighted');
                }
            }
        },
        init: function(){
            var _self = this;

            if (!$('.relatorio-page').length)
                return;

            $.ajax({
                url: '/mopa/public/georeport/v2/requests.json?start_date='+defaultStartDate,
                success: function(res){
                    _self.res = res;
                    _self.hooks();
                    _self.parse();
                    _self.inited = true;
                }
            });
        }
    };

    $(document).ready(function(){
        HomePageStats.init();
        ChartsPage.init();
        InqueriesSent.init();
        InqueriesReceive.init();

        $('.field-label').addClass('label');

        $('.geolocation-address-geocode, .geolocation-client-location, .geolocation-remove').addClass('btn');

        // Hide form input's address on focus.
        $('.geolocation-address input').focus(function(){
            this.value = '';
        });

        $('.node-report-form #edit-submit').html(Drupal.t('Next'));

        if (window.location.pathname.substr(window.location.pathname.length - 4) == 'edit')  {
                $('.node-report-form #edit-submit').html(Drupal.t('Save'));
        }

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

            if (linkHash.indexOf('4--') > -1 || linkHash.indexOf('---foto') > -1 ||  window.location.pathname.substr(window.location.pathname.length - 4) == 'edit')  {
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

            if ((!currentHash || currentHash.indexOf('1--') > -1 || currentHash.indexOf('---local') > -1) && !(window.location.pathname.substr(window.location.pathname.length - 4) == 'edit')) {
		$('a:contains(2.)').tab('show');
                var hash = $('a:contains(2.)').attr('href');
                // animate
                $('html, body').animate({
                    scrollTop: $(hash).offset().top - 30
                    }, 600, function(){
                    window.location.hash = hash;
                });
            }
            else if (currentHash.indexOf('2--') > -1 || currentHash.indexOf('---relatorio') > -1) {
                $('a:contains(3.)').tab('show');

                var hash = $('a:contains(3.)').attr('href');
                // animate
                $('html, body').animate({
                    scrollTop: $(hash).offset().top - 30
                    }, 600, function(){
                    window.location.hash = hash;
                });
                document.getElementById("edit-field-geo-und-0-address-field").disabled = false;
            }
            else if (currentHash.indexOf('3--') > -1 || currentHash.indexOf('---contacto') > -1) {
                $('a:contains(4.)').tab('show');

                var hash = $('a:contains(4.)').attr('href');
                // animate
                $('html, body').animate({
                    scrollTop: $(hash).offset().top - 30
                    }, 600, function(){
                    window.location.hash = hash;
                });
                document.getElementById("edit-field-geo-und-0-address-field").disabled = false;
                $('#edit-submit').html(Drupal.t('Save'));
            }

            else if (currentHash.indexOf('4--') > -1 || currentHash.indexOf('---foto') > -1 || window.location.pathname.substr(window.location.pathname.length - 4) == 'edit' ) {
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
    
    $(window).load(function(){
        $('#loading').fadeOut(0);
    });
})(jQuery);
