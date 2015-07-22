Bloom filter implemented in C++
===============================

It's a [Bloom filter](https://en.wikipedia.org/wiki/Bloom_filter), implemented
in C++ for your JavaScript pleasure.

Quick refresher: a bloom filter has two methods: `add()` and `test()`. You
`add()` an element (a utf-8 string, in this case) and voodoo happens; you
`test()` for an element and it returns `false` if the element is _definitely
not_ in the set, or `true` if the element is _probably_ in the set.

With the right parameters, you can tweak "probably" to be pretty darned
probable.

Usage
-----

First, `npm install overview-js-bloom-filter`.

Now:

```javascript
var filter = new BloomFilter({ m: 1234567, k: 3 });

// Add some Strings
filter.add('foo'); // UCS-2 / UTF16-LE String: 6 bytes long
filter.add(new Buffer('bar', 'utf-8')); // UTF-8 String: 3 bytes long

// Test for things: Strings...
console.log(filter.test('foo')); // true
console.log(filter.test('baz')); // false
console.log(filter.test('bar')); // false: 'bar' is UTF-16

// A Buffer...
console.log(filter.test(new Buffer('bar', 'utf-8'))); // true

// And even a _piece_ of a Buffer, to avoid a `slice()` object allocation
console.log(filter.test(new Buffer('xxxbarxxx', 'utf-8'), 3, 6); // true

// And of course, you can save it to disk
var buf = filter.serialize(); // a Buffer
var restoredFilter = BloomFilter.load(buf); // a BloomFilter
```

Confused about what `m` and `k` should be? Try
[http://hur.st/bloomfilter](http://hur.st/bloomfilter), which plugs numbers
into the formulae.

Implementation details
----------------------

It's a basic bloom filter. It's built to be fast, so:

* The hashing function is [Farmhash](https://farmhash.googlecode.com), because
  it's fast and well-tested.
* It's in C++ because Farmhash is in C++.
* We hash the string as 64-bit, then treat the result as two independent 32-bit
  hashes. All but the first two hashes are derived from the first two. [Two is
  enough.](http://www.eecs.harvard.edu/~michaelm/postscripts/rsa2008.pdf)
* We handle `Buffer`s because they let you write fast JavaScript.

LICENSE
-------

AGPL-3.0. This project is (c) Overview Services Inc. Please contact us should
you desire a more permissive license.
