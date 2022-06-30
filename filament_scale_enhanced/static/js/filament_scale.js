/*
 * View model for OctoPrint-Filament_scale
 *
 * Author: Victor Noordhoek, Erik Hess
 * License: AGPLv3
 */
 
 
 
$(function() {
    function Filament_scaleViewModel(parameters) {
        var self = this;

		self.printerState = parameters[0]
		self.settings = parameters[1]

		self.last_raw_weight = 0
		self.calibrate_known_weight = 0

		self.printerState.filamentRemainingString = ko.observable("Loading...")

		self.onDataUpdaterPluginMessage = function(plugin, message) {
			if (plugin != "filament_scale") return;
				
			self.last_raw_weight = parseInt(message)
			
			weight = message
			if (Number.isNaN(weight)){
				error_message = {"offset": self.settings.settings.plugins.filament_scale.offset(),
								"cal_factor": self.settings.settings.plugins.filament_scale.cal_factor(),
								"message" : message,
								"known_weight": self.calibrate_known_weight,
								"spool_weight": self.settings.settings.plugins.filament_scale.spool_weight()}
				console.log(error_message)
				// self.settings.settings.plugins.filament_scale.lastknownweight("Error")
				self.printerState.filamentRemainingString("Calibration Error")				 
					self.printerState.filamentRemainingString("Calibration Error")				 
				self.printerState.filamentRemainingString("Calibration Error")				 
			} else{
				weight_after_spool = weight - self.settings.settings.plugins.filament_scale.spool_weight()
				self.settings.settings.plugins.filament_scale.lastknownweight(weight_after_spool)
				self.printerState.filamentRemainingString(weight_after_spool + "g")
			}
		};

		self.onStartup = function() {
            var element = $("#state").find(".accordion-inner [data-bind='text: stateString']");
            if (element.length) {
                var text = gettext("Filament Remaining");
                element.after("<br>" + text + ": <strong data-bind='text: filamentRemainingString'></strong>");
            }
        };

		self.tare = function() {
			self.sendReq("tare", 0, function() {})
		};

		self.calibrate = function() {
			self.sendReq("calibrate", self.calibrate_known_weight, function() {})
		};

		self.sendReq = function(command, value, fn, errCb) {
			$.ajax({
				url: `/api/plugin/filament_scale?command=${command}&value=${value}`,
				type: 'GET',
				dataType: 'text',
				error: function() {
					if (errCb) {
						errCb()
					}
				}
			}).done((c)=>{if(fn){fn(c)}})
		};
		
	}
	
    /* view model class, parameters for constructor, container to bind to
     * Please see http://docs.octoprint.org/en/master/plugins/viewmodels.html#registering-custom-viewmodels for more details
     * and a full list of the available options.
     */
    OCTOPRINT_VIEWMODELS.push({
        construct: Filament_scaleViewModel,
        // ViewModels your plugin depends on, e.g. loginStateViewModel, settingsViewModel, ...
        dependencies: ["printerStateViewModel", "settingsViewModel"],
        // Elements to bind to, e.g. #settings_plugin_filament_scale, #tab_plugin_filament_scale, ...
        elements: ["#settings_plugin_filament_scale"]
    });
});
