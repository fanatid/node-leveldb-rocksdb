const test = require('tape')
const path = require('path')
const fs = require('fs-extra')
const addon = require('bindings')('node-leveldb-rocksdb')
const util = require('../lib/util')

const datadir = path.join(__dirname, 'addon-tmp-db')
const removeObject = () => fs.removeSync(datadir)
process.on('exit', removeObject)

testDB('LevelDB')

function testDB (name) {
  test(name, async (t) => {
    removeObject()

    const DB = addon[name]
    const db = new DB(datadir)
    db.open()
    // const db = require('leveldown')(datadir)
    // await util.promisify(db, 'open')()
    await util.promisify(db, 'batch')([
      { type: 'put', key: Buffer.from('hello'), value: Buffer.from('') }
    ])
    // db.batch([{ type: 'put', key: Buffer.from([0, 1]), value: Buffer.from([]) }])
    // const x = await util.promisify(db, 'get')(Buffer.from([1]))
    // console.log(x)
    const x = await util.promisify(db, 'getSeq')({
      gt: Buffer.from(''),
      lt: Buffer.from('~')
    })
    console.log(x)

    t.end()
  })
}
