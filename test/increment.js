var Promise = require('bluebird');
var fs = Promise.promisifyAll(require('fs'));
var assert = require('assert');
var device = fs.readFileSync(__dirname + '/../device').toString().trim();

function open(path, flags) {
    return fs.openAsync(path, flags).disposer(function (fd) {
        return fs.closeAsync(fd);
    });
}

describe('device', function () {
    it('increments a given integer', function () {
        var input = Math.round(Math.random() * 1000);
        var expectedOutput = input + 1;
        return Promise.using(open(device, 'r+'), function (fd) {
            var readBuffer = new Buffer(expectedOutput.toString().length);
            var writeBuffer = new Buffer(input + '\r');
            return Promise
                .resolve(fs.writeAsync(fd, writeBuffer, 0, writeBuffer.length))
                .then(function () {
                    fs.readSync(fd, readBuffer, 0, readBuffer.length);
                })
                .then(function () {
                    assert.equal(parseInt(readBuffer.toString(), 10), expectedOutput);
                });
        });
    });
});