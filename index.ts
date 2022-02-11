// eslint-disable-next-line @typescript-eslint/no-unsafe-assignment, @typescript-eslint/no-unsafe-call
const addon = require('bindings')('engine')

export interface Mora {
  text: string
  consonant?: string
  consonant_length?: number
  vowel: string
  vowel_length: number
  pitch: number
}

export interface AccentPhrase {
  moras: Mora[]
  accent: number
  pause_mora?: Mora
}

export interface AudioQuery {
  accent_phrases: AccentPhrase[]
  speedScale: number
  pitchScale: number
  intonationScale: number
  volumeScale: number
  prePhonemeLength: number
  postPhonemeLength: number
  outputSamplingRate: number
  outputStereo: boolean
  kana: string
}

interface IEngine {
  audio_query(text: string, speaker_id: number): AudioQuery
  accent_phrases(
    text: string,
    speaker_id: number,
    is_kana?: boolean
  ): AccentPhrase[]
  mora_data(accent_phrases: AccentPhrase[], speaker_id: number): AccentPhrase[]
  mora_length(
    accent_phrases: AccentPhrase[],
    speaker_id: number
  ): AccentPhrase[]
  mora_pitch(accent_phrases: AccentPhrase[], speaker_id: number): AccentPhrase[]
  synthesis(
    audio_query: AudioQuery,
    speaker_id: number,
    enable_interrogative_upspeak?: boolean
  ): Buffer
  metas(): string
  yukarin_s_forward(phoneme_list: number[], speaker_id: number): number[]
  yukarin_sa_forward(
    vowel_phoneme_list: number[],
    consonant_phoneme_list: number[],
    start_accent_list: number[],
    end_accent_list: number[],
    start_accent_phrase_list: number[],
    end_accent_phrase_list: number[],
    speaker_id: number
  ): number[]
  decode_forward(
    f0: number[],
    phoneme: number[][],
    speaker_id: number
  ): number[]
}

/**
 * CoreとEngineの関数をまとめてラップしたクラス
 */
class Engine implements IEngine {
  private readonly addon: IEngine

  /**
   * Engineクラスの初期化
   * Coreライブラリはライセンスの関係上、使用者自身で用意する必要があるため、
   * Coreライブラリのパス(ファイル名まで含む)を指定する。
   * また、音声ライブラリデータ群を別フォルダ内で保持している場合は、そのフォルダのパスを指定する。
   * 読み込みに失敗した場合、エラーを投げるので、try-catchでのエラーハンドリングを推奨。
   * @param {string} coreFilePath - Coreライブラリのパス(絶対パス推奨)
   * @param {boolean} useGpu - GPUを使うか否か
   * @param {string} libraryRootDirPath - 音声ライブラリデータ群が存在するフォルダのパス(絶対パス推奨)
   */
  constructor(
    coreFilePath: string,
    useGpu: boolean,
    libraryRootDirPath?: string
  ) {
    // eslint-disable-next-line @typescript-eslint/no-unsafe-assignment, @typescript-eslint/no-unsafe-call
    this.addon = new addon(
      __dirname + '/open_jtalk_dic_utf_8-1.11',
      coreFilePath,
      useGpu,
      libraryRootDirPath
    )
  }

  /**
   * 音声合成用のクエリを作成する
   * クエリの初期値を得ます。
   * ここで得られたクエリはそのまま音声合成に利用できます。
   * @param {string} text - 音声合成用の文字列
   * @param {number} speaker_id - 話者ID
   * @return {AudioQuery} - 音声合成用のクエリ
   */
  audio_query(text: string, speaker_id: number): AudioQuery {
    return this.addon.audio_query(text, speaker_id)
  }

  /**
   * テキストからアクセント句を得る
   * @param {string} text - アクセント句を取得したい文字列
   * @param {number} speaker_id - 話者ID
   * @param {boolean} is_kana - AquesTalkライクな記法の文字列かどうか
   * @return {AccentPhrase[]} - アクセント句
   */
  accent_phrases(
    text: string,
    speaker_id: number,
    is_kana?: boolean
  ): AccentPhrase[] {
    return this.addon.accent_phrases(text, speaker_id, is_kana ?? false)
  }

  /**
   * アクセント句から音高・音素長を得る
   * @param {AccentPhrase[]} accent_phrases - アクセント句
   * @param {number} speaker_id - 話者ID
   * @return {AccentPhrase[]} - 編集されたアクセント句
   */
  mora_data(
    accent_phrases: AccentPhrase[],
    speaker_id: number
  ): AccentPhrase[] {
    return this.addon.mora_data(accent_phrases, speaker_id)
  }

  /**
   * アクセント句から音素長を得る
   * @param {AccentPhrase[]} accent_phrases - アクセント句
   * @param {number} speaker_id - 話者ID
   * @return {AccentPhrase[]} - 編集されたアクセント句
   */
  mora_length(
    accent_phrases: AccentPhrase[],
    speaker_id: number
  ): AccentPhrase[] {
    return this.addon.mora_length(accent_phrases, speaker_id)
  }

  /**
   * アクセント句から音高を得る
   * @param {AccentPhrase[]} accent_phrases - アクセント句
   * @param {number} speaker_id - 話者ID
   * @return {AccentPhrase[]} - 編集されたアクセント句
   */
  mora_pitch(
    accent_phrases: AccentPhrase[],
    speaker_id: number
  ): AccentPhrase[] {
    return this.addon.mora_pitch(accent_phrases, speaker_id)
  }

  /**
   * 音声合成をする
   * @param {AudioQuery} audio_query - 音声合成用のクエリ
   * @param {number} speaker_id - 話者ID
   * @param {boolean} enable_interrogative_upspeak - 疑問文対応
   * @return {Buffer} - 音声合成されたwav形式のバイナリ
   */
  synthesis(
    audio_query: AudioQuery,
    speaker_id: number,
    enable_interrogative_upspeak?: boolean
  ): Buffer {
    return this.addon.synthesis(
      audio_query,
      speaker_id,
      enable_interrogative_upspeak ?? true
    )
  }

  /**
   * メタ情報(話者名や話者IDのリスト)を取得する関数。
   * @return {string} - メタ情報
   */
  metas(): string {
    return this.addon.metas()
  }

  /**
   * 音素列(phoneme_list)から音素ごとの長さを求める関数。
   * @param {number[]} phoneme_list - 音素列
   * @param {number[]} speaker_id - 話者番号
   * @return {number[]} - 音素ごとの長さ
   */
  yukarin_s_forward(phoneme_list: number[], speaker_id: number): number[] {
    return this.addon.yukarin_s_forward(phoneme_list, speaker_id)
  }

  /**
   * モーラごとの音素列とアクセント情報から、モーラごとの音高を求める
   * @param {number[]} vowel_phoneme_list - 母音の音素列
   * @param {number[]} consonant_phoneme_list - 子音の音素列
   * @param {number[]} start_accent_list - アクセントの開始位置
   * @param {number[]} end_accent_list - アクセントの終了位置
   * @param {number[]} start_accent_phrase_list - アクセント句の開始位置
   * @param {number[]} end_accent_phrase_list - アクセント句の終了位置
   * @param {number} speaker_id - 話者番号
   * @return {number[][]} - モーラごとの音高
   */
  yukarin_sa_forward(
    vowel_phoneme_list: number[],
    consonant_phoneme_list: number[],
    start_accent_list: number[],
    end_accent_list: number[],
    start_accent_phrase_list: number[],
    end_accent_phrase_list: number[],
    speaker_id: number
  ): number[] {
    return this.addon.yukarin_sa_forward(
      vowel_phoneme_list,
      consonant_phoneme_list,
      start_accent_list,
      end_accent_list,
      start_accent_phrase_list,
      end_accent_phrase_list,
      speaker_id
    )
  }

  /**
   * フレームごとの音素と音高から、波形を求める
   * @param {number[]} f0 - フレームごとの音高
   * @param {number[][]} phoneme - フレームごとの音素
   * @param {number} speaker_id - 話者番号
   * @return {number[]} - 音声波形
   */
  decode_forward(
    f0: number[],
    phoneme: number[][],
    speaker_id: number
  ): number[] {
    return this.addon.decode_forward(f0, phoneme, speaker_id)
  }
}

export default Engine
