const { XorShift128Plus } = require('xorshift.js')

const SEED = '4dd7719850e75b4d7843e11f'
module.exports = new XorShift128Plus(SEED)
