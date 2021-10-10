// eslint-disable-next-line @typescript-eslint/no-unsafe-assignment, @typescript-eslint/no-unsafe-call
const addon = require('bindings')('core')

interface ICore {
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
  /* decode_forward(
    f0: number[][],
    phoneme: number[][],
    speaker_id: number[]
  ): number[] */
}

/**
 * Coreの関数をまとめてラップしたクラス
 */
class Core implements ICore {
  private readonly addon: ICore

  /**
   * Coreクラスの初期化
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
    this.addon = new addon(coreFilePath, useGpu, libraryRootDirPath)
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
   * @param {number[][]} f0 - フレームごとの音高
   * @param {number[][]} phoneme - フレームごとの音素
   * @param {number[]} speaker_id - 話者番号
   * @return {number[]} - 音声波形
   */
  /* decode_forward(
    f0: number[][],
    phoneme: number[][],
    speaker_id: number[]
  ): number[] {
    return this.addon.decode_forward(f0, phoneme, speaker_id)
  } */
}

export default Core
