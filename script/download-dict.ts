import fetch from 'node-fetch'
import { stdout } from 'process'
import { extract } from 'tar'
import { createGunzip } from 'zlib'

const gunzip = createGunzip()
const extractor = extract({ path: '../open_jtalk_dic_utf_8-1.11' })

const dict_url =
  'https://downloads.sourceforge.net/open-jtalk/open_jtalk_dic_utf_8-1.11.tar.gz'

const now_downloading = 'Donwloading OpenJTalk Dictionary...'

extractor.on('finish', () => {
  console.log('\nDonwload Complete!')
})
process.stdout.write(now_downloading + '\r')
void fetch(dict_url).then((res) => {
  const binary = res.body
  if (binary === null) {
    throw Error('Failed download OpenJTalk Dictionary')
  }
  const length = Number(res.headers.get('content-length')!)
  let now_size = 0
  binary.on('data', (data: Buffer) => {
    now_size += data.length
    process.stdout.write(
      `${now_downloading} ${Math.floor((now_size * 100) / length)}%\r`
    )
  })

  binary.pipe(gunzip).pipe(extractor)
})
