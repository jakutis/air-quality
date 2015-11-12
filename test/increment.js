var Promise = require('bluebird');
var fs = Promise.promisifyAll(require('fs'));
var assert = require('assert');
var debug = require('debug')('air-quality');
var serialport = require('serialport');
var _ = require('lodash');
var through = require('through');
var readline = require('readline');

assert(process.env.DEVICE);

function open(path, flags) {
    return fs.openAsync(path, flags).disposer(function (fd) {
        return fs.closeAsync(fd);
    });
}

function getSerialPort(path, opts) {
    return Promise
        .resolve(Promise.promisifyAll(_.bindAll(new serialport.SerialPort(path, _.pick(opts, [
            'baudRate', 'dataBits', 'stopBits', 'parity', 'rtscts', 'xon', 'xoff',
            'xany', 'flowControl', 'bufferSize', 'platformOptions'
        ]), false))))
        .then(function (port) {
            return new Promise(function (resolveOpen, rejectOpen) {
                port.on('open', function () {
                    port.pause();
                    var resolve, reject, start, finish, handler;
                    function getState() {
                        if (resolve === undefined || finish) {
                            return 'finished';
                        }
                        if (start) {
                            return 'started';
                        }
                        if (handler) {
                            return 'initialized';
                        }
                        return 'uninitialized';
                    }
                    port.on('error', function (error) {
                        if (resolve === undefined) {
                            return;
                        }
                        resolve = undefined;

                        reject(error);
                        port.close();
                    });
                    port.on('close', function () {
                        if (resolve === undefined) {
                            return;
                        }
                        var _resolve = resolve;
                        resolve = undefined;
                        _resolve();
                    });
                    resolveOpen({
                        result: new Promise(function (_resolve, _reject) {
                            resolve = _resolve;
                            reject = _reject;
                        }),
                        initialize: function (_handler) {
                            if (getState() !== 'uninitialized') {
                                return Promise.reject(new Error('expected to be uninitialized, but it is ' + getState()));
                            }
                            if (typeof _handler !== 'function') {
                                return Promise.reject(new Error('handler is not a function'));
                            }
                            handler = _handler;
                            return Promise.resolve();
                        },
                        start: function () {
                            if (getState() !== 'initialized') {
                                return Promise.reject(new Error('expected to be initialized, but it is ' + getState()));
                            }
                            port.on('data', function (data) {
                                if (getState() !== 'started') {
                                    return;
                                }
                                try {
                                    handler(data);
                                } catch (err) {
                                    resolve = undefined;
                                    reject(err);
                                    port.close();
                                }
                            });
                            port.resume();
                            return start = Promise.resolve();
                        },
                        finish: function () {
                            if (getState() !== 'started') {
                                return Promise.reject(new Error('expected to be started, but it is ' + getState()));
                            }
                            return finish = port.closeAsync();
                        },
                        drain: function () {
                            if (getState() !== 'started') {
                                return Promise.reject(new Error('expected to be started, but it is ' + getState()));
                            }
                            return port.drainAsync();
                        },
                        flush: function () {
                            if (getState() !== 'started') {
                                return Promise.reject(new Error('expected to be started, but it is ' + getState()));
                            }
                            return port.flushAsync();
                        },
                        write: function (data) {
                            if (getState() !== 'started') {
                                return Promise.reject(new Error('expected to be started, but it is ' + getState()));
                            }
                            if (!(data instanceof Buffer)) {
                                return Promise.reject(new Error('data is not a Buffer'));
                            }
                            return port.writeAsync(data);
                        }
                    });
                });
                port.on('error', rejectOpen);
                port.open();
            });
        })
        .disposer(function (port) {
            return port.finish().catch(_.noop);
        });
}

describe('device', function () {
    it('increments a given integer', function () {
        var input = Math.round(Math.random() * 1000);
        var expectedOutput = input + 1;
        var actualOutput;
        return Promise.using(getSerialPort(process.env.DEVICE, {
            baudRate: 9600
        }), function (port) {
            var t = through();
            readline.createInterface({
                input: t
            }).on('line', function (line) {
                actualOutput = parseInt(line, 10);
                port.finish();
            });
            Promise
                .bind({})
                .then(function () {
                    return port.initialize(function (chunk) {
                        t.write(chunk);
                    });
                })
                .then(function () {
                    return port.start().delay(2000);
                })
                .then(function () {
                    return port.write(new Buffer(input + '\r'));
                })
                .then(function () {
                    return port.drain();
                })
                .then(function () {
                    return port.flush();
                });
            return port.result.then(function () {
                debug('assert', actualOutput, expectedOutput);
                assert.equal(actualOutput, expectedOutput);
            });
        });
    });
});