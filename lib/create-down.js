const prng = require('./prng')
const util = require('./util')

module.exports = function (engine) {
  return async ({ datadir, keySize, keysCount }) => {
    const db = engine(datadir)

    // default options from https://github.com/Level/leveldown#leveldownopenoptions-callback
    await util.promisify(db, 'open')({
      createIfMissing: true,
      errorIfExists: true,
      compression: true,
      cacheSize: 8 * 1024 * 1024,
      writeBufferSize: 4 * 1024 * 1024,
      blockSize: 4096,
      maxOpenFiles: 1000,
      blockRestartInterval: 16,
      maxFileSize: 2 * 1024 * 1024
    })

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

          await batch(operations, { sync: false })
        }
      },

      async randUpdate () {
        for (let count = 0; count < updateKeys.length; count += 100) {
          const operations = new Array(Math.min(100, updateKeys.length - count))
          for (let i = 0; i < operations.length; ++i) {
            operations[i] = { type: 'put', key: updateKeys[count + i], value: util.buffer.one }
          }

          await batch(operations, { sync: false })
        }
      },

      async randRead () {
        const get = util.promisify(db, 'get')
        const options = { fillCache: true, asBuffer: true }
        for (let count = 0; count < readKeys.length; count += 5) {
          await Promise.all(readKeys.slice(count, count + 5).map((key) => get(key, options)))
        }
      },

      async seqRead () {
        // read 1000 keys, but not more than 5% of all keys
        for (let count = 0; count < keysCount * 0.05;) {
          const iterator = db.iterator({
            gt: updateKeys[~~(prng.random() * updateKeys.length)],
            lt: Buffer.alloc(keySize, 0xff),
            reverse: false,
            keys: true,
            values: true,
            limit: -1,
            fillCache: true,
            keyAsBuffer: true,
            valueAsBuffer: true
          })

          await new Promise((resolve, reject) => {
            function loop (err, key, value) {
              if (err) return iterator.end(() => reject(err))
              if (key === undefined && value === undefined) return iterator.end(resolve)

              count += 1
              iterator.next(loop)
            }

            iterator.next(loop)
          })
        }
      },

      async close () {
        await util.promisify(db, 'close')()
      }
    }
  }
}
