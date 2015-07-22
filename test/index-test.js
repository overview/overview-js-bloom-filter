var bf = require('../index');
var expect = require('chai').expect;

describe('BloomFilter', function() {
  it('should return true/false from tests appropriately', function() {
    var subject = new bf.BloomFilter(100, 4); // big enough
    subject.add("foo");
    subject.add("bar");

    expect(subject.test("foo")).to.be.truthy;
    expect(subject.test("bar")).to.be.truthy;
    expect(subject.test("baz")).to.be.falsy;
    expect(subject.test("foo2")).to.be.falsy;
  });

  it('should serialize to a buffer', function() {
    var subject = new bf.BloomFilter(10, 3);
    subject.add("foo");

    var buf = subject.serialize();
    expect(buf.length).to.eq(7); // 5 for the header, 2 for the contents
    expect(buf[0]).to.eq(10); // It's big-endian: network interchange format
    expect(buf[1]).to.eq(0);
    expect(buf[2]).to.eq(0);
    expect(buf[3]).to.eq(0);
    expect(buf[4]).to.eq(3);
    expect(buf[5]).to.be.greaterThan(0);
  });

  it('should deserialize a serialized buffer that is all ones', function() {
    var buf = new Buffer([
      0x0a, 0x00, 0x00, 0x00, // nBuckets=10, big-endian format
      0x03, // nHashes
      0xff, 0xc0 // all 1s
    ]);

    // It's all 1s, so everything will match
    var subject = bf.unserialize(buf);
    expect(subject.test("foo")).to.be.truthy;
    expect(subject.test("bar")).to.be.truthy;
  });

  it('should deserialize a serialized buffer that is all zeroes', function() {
    var buf = new Buffer([
      0x0a, 0x00, 0x00, 0x00, // nBuckets=10, big-endian format
      0x03, // nHashes
      0x00, 0x00 // all 0s
    ]);

    // It's all 0s, so nothing will match
    var subject = bf.unserialize(buf);
    expect(subject.test("foo")).to.be.falsy;
    expect(subject.test("bar")).to.be.falsy;
  });

  it('should work after serialize+deserialize', function () {
    var subject = new bf.BloomFilter(100, 4); // big enough
    subject.add("foo");
    subject.add("bar");
    subject = bf.unserialize(subject.serialize());

    expect(subject.test("foo")).to.be.truthy;
    expect(subject.test("bar")).to.be.truthy;
    expect(subject.test("baz")).to.be.falsy;
    expect(subject.test("foo2")).to.be.falsy;
  });

  it('should allow testing for utf8-encoded Buffer objects', function() {
    var subject = new bf.BloomFilter(100, 4); // big enough
    subject.add("foo");

    expect(subject.test(new Buffer("foo", "utf-8"))).to.be.truthy;
    expect(subject.test(new Buffer("foo", "ucs-2"))).to.be.falsy;
  });

  it('should allow writing Buffer objects', function() {
    var subject = new bf.BloomFilter(100, 4); // big enough
    subject.add(new Buffer("foo", "utf-8"));

    expect(subject.test("foo")).to.be.truthy;
  });

  it('should allow testing for slices of buffers', function() {
    var subject = new bf.BloomFilter(100, 4);
    subject.add("foo");

    expect(subject.test(new Buffer("xxxfooxxx", "utf-8"), 3, 6)).to.be.truthy;
    expect(subject.test(new Buffer("xxxfoo", "utf-8"), 3)).to.be.truthy;
    expect(subject.test(new Buffer("fooxxx", "utf-8"), 0, 3)).to.be.truthy;
  });

  it('should not slice past the end of the buffer', function() {
    var subject = new bf.BloomFilter(100, 4);
    subject.add("foo");

    expect(subject.test(new Buffer("xxxfoo", "utf-8"), 3, 200)).to.be.truthy;
  });

  it('should test an empty String if end<begin', function() {
    var subject = new bf.BloomFilter(100, 4);
    subject.add("foo");
    expect(subject.test(new Buffer("xxxfooxxx"), 4, 3)).to.be.falsy;
    subject.add("");
    expect(subject.test(new Buffer("xxxfooxxx"), 4, 3)).to.be.truthy;
  });
});
