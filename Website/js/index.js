var systemStatus = {};
$(function () {
    var self = this;

    self.$flexSwitchCheckHeater = $("#flexSwitchCheckHeater");
    self.$flexSwitchHeaterLabel = $("#flexSwitchHeaterLabel");
    self.$flexSwitchCheckPrinter = $("#flexSwitchCheckPrinter");
    self.$flexSwitchPrinterLabel = $("#flexSwitchPrinterLabel");
    self.isPolling = false;
    self.pollingCount = 0;
    self.pollingLimit = 1000;

    self.resetSystem = () => {
        $.get("//192.168.1.112/?ResetSystem", function (data) {
            setTimeout(() => {
                window.location.href = "/";
            }, 3000);
        });
    };

    self.getSystemStatus = () => {
        $.getJSON("//192.168.1.112/GetStatus", function (data) {
            systemStatus = data;
            $("#ambient-temp").html(data.temperature.ambient);
            $("#resin-temp").html(data.temperature.resin);

            self.$flexSwitchCheckHeater.prop('checked', data.isHeaterOn);
            self.$flexSwitchCheckPrinter.prop('checked', data.isPrinterOn);

            self.setHeaterStatusLabel();
            self.setPrinterStatusLabel();

            if (/*data.isPrinterOn &&*/ !self.isPolling) {
                self.isPolling = true;
                self.startPolling();
            }

            if (!data.isPrinterOn && self.pollingCount > self.pollingLimit) {
                self.resetSystem();
            }
        });
    }

    if (!self.isPolling) {
        self.getSystemStatus();
    }

    self.startPolling = () => {
        poll(() => new Promise((resolve) => {
            self.isPolling = true;
            self.pollingCount++;
            self.getSystemStatus();
            resolve(); //you need resolve to jump into .then()
        }), 5000, 10000);
    }

    self.cancelPolling = () => {
        cancelCallback();
        self.isPolling = false;
    }

    self.setHeaterStatusLabel = () => {
        if (self.$flexSwitchCheckHeater.is(':checked')) {
            self.$flexSwitchHeaterLabel.html(`Heater on <i class="fas fa-fan fa-spin"></i>`);
        }
        else {
            self.$flexSwitchHeaterLabel.html(`Heater off <i class="fas fa-fan"></i>`);
        }
    };
    self.setPrinterStatusLabel = () => {
        if (self.$flexSwitchCheckPrinter.is(':checked')) {
            self.$flexSwitchPrinterLabel.html(`Printer on <i class="fas fa-power-off text-success"></i>`);
        }
        else {
            self.$flexSwitchPrinterLabel.html(`Printer off <i class="fas fa-power-off text-danger"></i>`);
        }
    };

    self.$flexSwitchCheckHeater.on("change", (e) => {
        var value = self.$flexSwitchCheckHeater.is(':checked') ? "on" : "off";

        $.get("//192.168.1.112/?HeaterPower=" + value, function (data) {
            self.getSystemStatus();
        });

    });    

    self.$flexSwitchCheckPrinter.on("change", (e) => {
        var value = self.$flexSwitchCheckPrinter.is(':checked') ? "on" : "off";

        $.get("//192.168.1.112/?PrinterPower=" + value, function (data) {
            self.getSystemStatus();
        });
    });

    $("#btn-reset-system").on("click", function (e) {
        self.resetSystem();
        e.preventDefault();
    });

    $("#btn-stop-polling").on("click", function (e) {
        cancelCallback();

        e.preventDefault();
    });


});

let cancelCallback = () => { };

var sleep = (period) => {
    return new Promise((resolve) => {
        cancelCallback = () => {
            //console.log("Canceling...");
            // send cancel message...
            return resolve('Canceled');
        }
        setTimeout(() => {
            resolve("tick");
        }, period)
    })
}

var poll = (promiseFn, period, timeout) => promiseFn().then(() => {
    let asleep = async (period) => {
        let respond = await sleep(period);
        // if you need to do something as soon as sleep finished
        //console.log("sleep just finished, do something...");
        return respond;
    }


    // just check if cancelCallback is empty function, 
    // if yes, set a time out to run cancelCallback()
    if (cancelCallback.toString() === "() => {}") {
        //console.log("set timeout to run cancelCallback()")
        setTimeout(() => {
            cancelCallback()
        }, timeout);
    }

    asleep(period).then((respond) => {
        // check if sleep canceled, if not, continue to poll
        if (respond !== 'Canceled') {
            poll(promiseFn, period);
        } else {
            console.log(respond);
        }
    })
})