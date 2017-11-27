const addon = require('bindings')('node-leveldb-rocksdb')
const prng = require('./prng')
const util = require('./util')

module.exports = function (engine) {
  return async ({ datadir, keySize, keysCount }) => {
    const Engine = addon[engine]
    const db = new Engine(datadir)
    db.open()

    const batch = util.promisify(db, 'batch')
    const updateKeys = [] // 1% of all keys
    const readKeys = [] // 0.2% of all keys

    return {
      async create () {
        for (let count = 0; count < keysCount; count += 100) {
          const operations = new Array(Math.min(100, keysCount - count))
          for (let i = 0; i < operations.length; ++i) {
            operations[i] = { type: 'put', key: prng.randomBytes(keySize), value: util.buffer.zero }
          }

          const getKey = () => operations[~~(prng.random() * operations.length)].key
          updateKeys.push(getKey())
          if (count % 500 === 0) readKeys.push(getKey())

          await batch(operations)
        }
      },

      async randUpdate () {
        /*
        for (let count = 0; count < updateKeys.length; count += 100) {
          const operations = new Array(Math.min(100, updateKeys.length - count))
          for (let i = 0; i < operations.length; ++i) {
            operations[i] = { type: 'put', key: updateKeys[count + i], value: util.buffer.one }
          }

          await batch(operations, { sync: false })
        }
        */
      },

      async randRead () {
        /*
        const get = util.promisify(db, 'get')
        for (let count = 0; count < readKeys.length; count += 5) {
          await Promise.all(readKeys.slice(count, count + 5).map((key) => get(key)))
        }
        */
      },

      async seqRead () {
        // read 1000 keys, but not more than 5% of all keys
        for (let count = 0; count < keysCount * 0.05;) {
          const [items] = await util.promisify(db, 'getSeq')({
            gt: updateKeys[~~(prng.random() * updateKeys.length)],
            lt: Buffer.alloc(keySize, 0xff)
          })
          count += items.length
        }
      },

      async close () {
        /*
        await util.promisify(db, 'close')()
        */
      }
    }
  }
}
