const yargs = require('yargs')
const fs = require('fs-extra')
const chalk = require('chalk')

;(async () => {
  const { argv } = yargs
    .option('engine', { type: 'string' })
    .option('key-size', { type: 'number' })
    .option('keys-count', { type: 'number' })
    .option('datadir', { type: 'string' })

  const engine = require(`./${argv.engine}`)
  const db = await engine(argv)

  const ets = []
  for (const fn of ['create', 'randUpdate', 'randRead', 'seqRead']) {
    const st = process.hrtime()
    await db[fn]()
    const et = process.hrtime(st)
    const etf = (et[0] + et[1] * 1e-9).toFixed(9) + 's'
    console.log(`${argv.engine}: ${fn} finished with ${etf}`)
    ets.push(etf)
  }

  await db.close()

  console.log(argv.engine + ': ' + chalk.red(`remove temporary directory: ${argv.datadir}`))
  await fs.remove(argv.datadir)

  process.send([argv.engine, ...ets])
})().catch((err) => {
  console.log(chalk.yellow(err.stack || err))
  process.exit(1)
})
