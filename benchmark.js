const path = require('path')
const os = require('os')
const cp = require('child_process')
const yargs = require('yargs')
const fs = require('fs-extra')
const chalk = require('chalk')
const table = require('text-table')

const { argv } = yargs
  .usage('Usage: $0 [options]')
  .option('key-size', {
    default: 32,
    describe: 'Key size in bytes',
    type: 'number'
  })
  .option('keys-count', {
    default: 100 * 1024 * 32, // 100 MiB for sum all keys with default key size
    describe: 'Keys count',
    type: 'number'
  })
  .option('datadir', {
    default: path.join(__dirname, 'tmp-db'),
    describe: 'Path to temporary directory for database',
    type: 'string',
    coerce: path.resolve
  })
  .option('engines', {
    default: 'leveldown,rocksdown,leveldb,rocksdb',
    describe: 'Engines for tests devided by comma',
    type: 'string',
    coerce: (value) => {
      const available = ['leveldown', 'rocksdown', 'leveldb', 'rocksdb']
      const engines = value.split(',').map((s) => s.trim())
      for (const engine of engines) {
        if (!available.includes(engine)) throw new Error(`Test for engine '${engine}' is not defined`)
      }
      return engines
    }
  })
  .fail((msg, err, yargs) => {
    console.log(chalk.red(msg))
    console.log('\n---')
    console.log(yargs.help())
    process.exit(1)
  })
  .help('help').alias('help', 'h')

;(async () => {
  const exists = await fs.pathExists(argv.datadir)
  if (exists) {
    console.log(chalk.yellow(`${argv.datadir} already exists...`))
    process.stdout.write(chalk.red('Remove? '))
    await new Promise((resolve, reject) => {
      process.stdin.once('data', (value) => {
        if (/^(y|yes)$/i.test(value.toString().trim())) resolve()
        else reject(new Error('Can not use data directory'))
      })
    })
    await fs.remove(argv.datadir)
    console.log('')
  }

  const result = []
  for (const engine of argv.engines) {
    const data = await new Promise((resolve, reject) => {
      const args = [
        '--max-old-space-size=4096',
        '--engine', engine,
        '--datadir', argv.datadir,
        '--key-size', argv.keySize,
        '--keys-count', argv.keysCount
      ]
      const proc = cp.fork(path.join(__dirname, 'lib', 'test.js'), args)
      proc.once('error', reject)
      proc.once('exit', (code, signal) => reject(new Error(`Exit without message with ${code || signal}`)))
      proc.once('message', resolve)
    })
    result.push(data)
  }

  console.log('\n---\n' + table(
    [['engine', 'create', 'randUpdate', 'randRead', 'seqRead'], ...result],
    { align: ['l', 'r', 'r', 'r', 'r'] }
  ))

  console.log('\nPlatform info:')
  console.log([os.type(), os.release(), os.arch()].join(' '))
  console.log(`Node.JS ${process.versions.node}`)
  console.log(`V8 ${process.versions.v8}`)
  const cpus = os.cpus()
    .map((cpu) => cpu.model)
    .reduce((o, model) => Object.assign(o, { [model]: (o[model] || 0) + 1 }), {})
  console.log(Object.keys(cpus).map((cpu) => `${cpu} \u00d7 ${cpus[cpu]}`).join('\n'))

  process.exit(0)
})().catch((err) => {
  console.log(chalk.red(err.stack || err))
  process.exit(1)
})
