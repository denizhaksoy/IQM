// Writes a factual day summary only after required acceptance artifacts pass.
const fs=require('node:fs'),path=require('node:path');
const [output]=process.argv.slice(2);if(!output||fs.existsSync(output))throw Error('Usage: fresh-report.md');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const fullRoot='validation/live_hdr_video_20261101_run01',lowRoot='validation/live_hdr_540p_video_20261101_run01';
const full=read(fullRoot+'/video_score.json'),low=read(lowRoot+'/video_score.json');
for(const root of [fullRoot,lowRoot]){const s=read(root+'/video_score.json'),a=read(root+'/acceptance.json'),o=read(root+'/aggregation_octave.json');if(!s.complete||!s.exact||s.frames!==16||!a.exact_all_selected_frames||!o.exact)throw Error('Video acceptance missing: '+root);}
for(const root of ['validation/public_cpu_540p_20261101_run01','validation/public_cpu_bonfire720_20261101_run01'])if(!read(root+'/comparison.json').exact)throw Error('Public CLI score mismatch');
const bonfire=read('validation/public_cpu_bonfire720_20261101_run01/score.json');
const checks=read('validation/public_cpu_cli_checks_20261101_run01/checks.json');if(!checks.passed||checks.cases.some(c=>!c.passed))throw Error('CLI checks failed');
const gpuRoot='validation/live_hdr_gpu4k_library_20261101_run01',gpuRun=read(gpuRoot+'/run.json');
const stages=fs.readFileSync(gpuRoot+'/results/results.jsonl','utf8').trim().split('\n').map(JSON.parse);
if(gpuRun.exit!==0||gpuRun.reportedValidationErrors||stages.length!==4||stages.some(s=>s.bit_mismatches))throw Error('GPU checks failed');
const gpuValues=stages.reduce((n,s)=>n+s.values,0),cases=full.frames+low.frames+1;
const cohort=read('validation/live_hdr_public_candidate_20261101_run01/candidate_cohort.json');
const summary={date:'2026-11-01',timezone:'Europe/Prague',scope:'measured local LIVE HDR quality validation; not full metric or original challenge completion',full_videos:[{video:'4k_3M_Daylight1',frames:full.frames,Q_JOD:full.cpp_Q_JOD,root:fullRoot},{video:'540p_2.2M_Daylight1',frames:low.frames,Q_JOD:low.cpp_Q_JOD,root:lowRoot}],additional_frame:{video:'720p_2.6M_Bonfire',frame:0,Q:bonfire.Q_text,Q_JOD:bonfire.Q_JOD_text},distinct_frame_pairs_exact:cases,gpu_matrix_values_exact:gpuValues,gpu_validation_errors_reported:false,cli_checks_passed:checks.cases.length,candidate_cohort_cases:cohort.cases.length,candidate_cohort_author_verified:false,correlation_performed:false,full_gpu_pipeline:false};
const markdown=`# HDR-VDP-3 günlük çalışma çıktısı — 1 Kasım 2026

Bugün doğrulanan kapsam, yerel olarak belgelenmiş LIVE HDR protokolü ve resmî HDR-VDP-3 v3.0.7 CPU Octave referansıdır. Tüm veri kümesinin veya orijinal yarışma gönderiminin tamamlandığı iddia edilmiyor.

## Ölçülen sonuçlar

| Test | Kapsam | Resmî uygulama ve C++ sonucu |
|---|---|---|
| 4k_3M_Daylight1 | 16/16 seçili kare, tam video örneklemesi | Video Q_JOD=${full.cpp_Q_JOD} |
| 540p_2.2M_Daylight1 → 4K | 16/16 seçili kare, tam video örneklemesi | Video Q_JOD=${low.cpp_Q_JOD} |
| 720p_2.6M_Bonfire → 4K | Sıfır tabanlı kare 0 | Q=${bonfire.Q_text}; Q_JOD=${bonfire.Q_JOD_text} |

Toplam **${cases} farklı gerçek HDR kare çiftinde hem Q hem Q_JOD bit düzeyinde eşleşti**. Tekrar çalıştırmaları bu sayıya eklenmedi. İki tam video testi aynı içeriğin farklı çözünürlük/bitrate sürümleridir; Bonfire testi ikinci içerikte tek kareyi kapsar.

Her tam video çiftinde 480 karenin zaman damgaları ve sayıları eşleştirildi. Sıfır tabanlı 0:30:450 seçildi. Video ortalaması kare başına Q_JOD üzerinden sıralı Single aritmetik ile hesaplandı ve Octave ile ayrıca doğrulandı. Başlangıç fazı 0 ve RGB48 PNG kod çözümü açık yerel tercihlerdir; yazarların özgün PNG hassasiyeti ve fazı doğrulanmış değildir.

## Uygulamada tamamlananlar

- Single kalite zinciri, yeniden kullanılabilir kütüphane API'sine bağlandı: renk dönüşümü, optik filtreleme, adaptasyon, JND, pyramid, maskeleme/CSF ve skor hesabı.
- Ana CMake projesine **IQM-hdrvdp3-cpu** eklendi. Backend açıkça CPU/FFTW'dir; giriş kodlaması ve model/FFTW yolları açıkça istenir. Mevcut çok metrikli IQM uygulamasının GPU profilleri değiştirilmedi.
- Yeni CLI, 540p ve Bonfire 720p gerçek görüntülerinde referansla birebir doğrulandı. ${checks.cases.length} argüman/çıktı koruma kontrolü geçti.
- PQ alanında Single bicubic büyütme, kırpma ve ardından fiziksel birimlere dönüşüm doğrulandı; referans 4K geometrisi korundu.
- Kütüphaneye iki açık Vulkan matris aşaması eklendi. 4K renk ve LMSR dönüşümlerinde **${gpuValues.toLocaleString('en-US')} değer, sıfır bit farkı** verdi. Vulkan validation hatası raporlanmadı. Bu, tam GPU kalite zinciri veya hızlanma iddiası değildir.
- Büyük tamponlar için 2D dispatch uygulandı; önceki shader/binary dosyaları korundu. Yalnızca yeni shader derlendi.
- ${cohort.cases.length} bozulmuş videoluk, üç referans içerikli aday değerlendirme listesi ve bright/dark etiket eşleşmeleri hazırlandı. Özgün train_video_info.csv ile doğrulanmadığı için yazarların kesin kohortu olarak sunulmuyor.

## Henüz tamamlanmayanlar

- Tüm 310 dosyada sayısal eşdeğerlik ve daha geniş içerik/izleme koşulu kapsamı.
- Yeterli sayıda tam video üzerinde, açık kohort ve referans dışlama politikasıyla subjektif korelasyon. Şu anda SROCC/PLCC başarısı iddia edilmiyor; MOS/DMOS ve Q_JOD aynı nicelikler değildir.
- Genel optik FFT, adaptasyon, JND ve kalite zincirinin tamamının Vulkan üzerinde çalışması. Bugünkü GPU kabulü iki matris aşamasıyla sınırlıdır.
- Orijinal yarışmanın özel test kohortu, kod çözüm hassasiyeti, çalıştırma ortamı ve yayımlanmış video tahmin vektörüyle birebir tekrar.

## Kanıt dosyaları

- [Tam 4K video](LIVE_HDR_VIDEO_PAIR_VALIDATION.md): ${fullRoot}
- Tam 540p video: ${lowRoot}; video_score.json, acceptance.json, aggregation_octave.json
- [CPU CLI ve 4K GPU aşamaları](PUBLIC_SINGLE_CPU_AND_GPU4K.md)
- [540p giriş ön işlemesi](LIVE_HDR_540P_VALIDATION.md)
- [Protokol ve kaynak farklılıkları](LIVE_HDR_PROTOCOL_AUDIT.md)

Sayısal kayıtlar, input/model/executable hash'leri ve başarısız eski denemeler korundu. Commit/push yapılmadı. Bu çıktı haftalık rapor için doğrulanmış çalışma kaydıdır; haftalık rapor veya e-posta henüz gönderilmedi.
`;
fs.writeFileSync(output,markdown);fs.writeFileSync(output.replace(/\.md$/,'.json'),JSON.stringify(summary,null,2)+'\n');console.log(output);
