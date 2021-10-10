import Engine from '@/index'

const engine = new Engine('core.dll', false)
console.log(engine.metas())
console.log(
  engine.yukarin_s_forward(
    [
      0, 14, 14, 8, 21, 21, 36, 21, 21, 12, 21, 21, 21, 21, 14, 15, 40, 22, 21,
      21, 0,
    ],
    1
  )
)
