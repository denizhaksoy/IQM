# HDR-VDP-3 günlük çalışma çıktısı — 1 Kasım 2026

Bugün doğrulanan kapsam, yerel olarak belgelenmiş LIVE HDR protokolü ve resmî HDR-VDP-3 v3.0.7 CPU Octave referansıdır. Tüm veri kümesinin veya orijinal yarışma gönderiminin tamamlandığı iddia edilmiyor.

## Ölçülen sonuçlar

| Test | Kapsam | Resmî uygulama ve C++ sonucu |
|---|---|---|
| 4k_3M_Daylight1 | 16/16 seçili kare, tam video örneklemesi | Video Q_JOD=8.4828519821167 |
| 540p_2.2M_Daylight1 → 4K | 16/16 seçili kare, tam video örneklemesi | Video Q_JOD=8.534526824951172 |
| 720p_2.6M_Bonfire → 4K | Sıfır tabanlı kare 0 | Q=7.088564395904541; Q_JOD=7.9553570747375488 |

Toplam **33 farklı gerçek HDR kare çiftinde hem Q hem Q_JOD bit düzeyinde eşleşti**. Tekrar çalıştırmaları bu sayıya eklenmedi. İki tam video testi aynı içeriğin farklı çözünürlük/bitrate sürümleridir; Bonfire testi ikinci içerikte tek kareyi kapsar.

Her tam video çiftinde 480 karenin zaman damgaları ve sayıları eşleştirildi. Sıfır tabanlı 0:30:450 seçildi. Video ortalaması kare başına Q_JOD üzerinden sıralı Single aritmetik ile hesaplandı ve Octave ile ayrıca doğrulandı. Başlangıç fazı 0 ve RGB48 PNG kod çözümü açık yerel tercihlerdir; yazarların özgün PNG hassasiyeti ve fazı doğrulanmış değildir.

## Uygulamada tamamlananlar

- Single kalite zinciri, yeniden kullanılabilir kütüphane API'sine bağlandı: renk dönüşümü, optik filtreleme, adaptasyon, JND, pyramid, maskeleme/CSF ve skor hesabı.
- Ana CMake projesine **IQM-hdrvdp3-cpu** eklendi. Backend açıkça CPU/FFTW'dir; giriş kodlaması ve model/FFTW yolları açıkça istenir. Mevcut çok metrikli IQM uygulamasının GPU profilleri değiştirilmedi.
- Yeni CLI, 540p ve Bonfire 720p gerçek görüntülerinde referansla birebir doğrulandı. 7 argüman/çıktı koruma kontrolü geçti.
- PQ alanında Single bicubic büyütme, kırpma ve ardından fiziksel birimlere dönüşüm doğrulandı; referans 4K geometrisi korundu.
- Kütüphaneye iki açık Vulkan matris aşaması eklendi. 4K renk ve LMSR dönüşümlerinde **116,121,600 değer, sıfır bit farkı** verdi. Vulkan validation hatası raporlanmadı. Bu, tam GPU kalite zinciri veya hızlanma iddiası değildir.
- Büyük tamponlar için 2D dispatch uygulandı; önceki shader/binary dosyaları korundu. Yalnızca yeni shader derlendi.
- 27 bozulmuş videoluk, üç referans içerikli aday değerlendirme listesi ve bright/dark etiket eşleşmeleri hazırlandı. Özgün train_video_info.csv ile doğrulanmadığı için yazarların kesin kohortu olarak sunulmuyor.

## Henüz tamamlanmayanlar

- Tüm 310 dosyada sayısal eşdeğerlik ve daha geniş içerik/izleme koşulu kapsamı.
- Yeterli sayıda tam video üzerinde, açık kohort ve referans dışlama politikasıyla subjektif korelasyon. Şu anda SROCC/PLCC başarısı iddia edilmiyor; MOS/DMOS ve Q_JOD aynı nicelikler değildir.
- Genel optik FFT, adaptasyon, JND ve kalite zincirinin tamamının Vulkan üzerinde çalışması. Bugünkü GPU kabulü iki matris aşamasıyla sınırlıdır.
- Orijinal yarışmanın özel test kohortu, kod çözüm hassasiyeti, çalıştırma ortamı ve yayımlanmış video tahmin vektörüyle birebir tekrar.

## Kanıt dosyaları

- [Tam 4K video](LIVE_HDR_VIDEO_PAIR_VALIDATION.md): validation/live_hdr_video_20261101_run01
- Tam 540p video: validation/live_hdr_540p_video_20261101_run01; video_score.json, acceptance.json, aggregation_octave.json
- [CPU CLI ve 4K GPU aşamaları](PUBLIC_SINGLE_CPU_AND_GPU4K.md)
- [540p giriş ön işlemesi](LIVE_HDR_540P_VALIDATION.md)
- [Protokol ve kaynak farklılıkları](LIVE_HDR_PROTOCOL_AUDIT.md)

Sayısal kayıtlar, input/model/executable hash'leri ve başarısız eski denemeler korundu. Commit/push yapılmadı. Bu çıktı haftalık rapor için doğrulanmış çalışma kaydıdır; haftalık rapor veya e-posta henüz gönderilmedi.
