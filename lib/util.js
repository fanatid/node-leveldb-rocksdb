function promisify (obj, fn) {
  return (...args) => {
    return new Promise((resolve, reject) => {
      obj[fn](...args, (err, ...args) => {
        if (err) reject(err)
        else resolve(args)
      })
    })
  }
}

module.exports = {
  promisify,
  buffer: {
    zero: Buffer.from([]),
    one: Buffer.from([1])
  }
}
