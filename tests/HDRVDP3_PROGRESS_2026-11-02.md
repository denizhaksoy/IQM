# HDR-VDP-3 — 2 Kasım 2026 ilerleme kaydı

Bugünkü ana sonuç, ikinci içerikte tam video eşdeğerliği ve gerçek GPU matris
aşamalarının uçtan uca hibrit kalite hesabına bağlanmasıdır. Tam veri kümesi veya
tam GPU uygulaması henüz tamamlanmış değildir.

## Sonraki tamamlanan değerlendirme

**Son kontrol:** `4k_15M_Bonfire` tam video testi **18/18** karede birebir geçti.
Video ortalaması **8.3020801544189453**, Single hex **4104d552**; bağımsız Octave
doğrulaması tamamlandı. Kohort **11/27 video, 207 kare**; Daylight1 dahil
**13 video çifti, 4 içerik ve 239 farklı HDR kare çifti** dosya/ortalama denetimini
geçti. Kayıt: `validation/live_hdr_evidence_20261102_run08`.

Public CPU ve hibrit temsilî kare denetiminde **5/27 video** iki arayüzde de
birebir geçti; her hibrit çalışmada dört gerçek Vulkan matris dispatch'i ve
raporlanmış validation hatası yok. Bu tekrarlar toplam 239 sayısına eklenmedi.
Sıradaki `4k_15M_CargoBoat` işleniyor; korelasyon denetleyicisi beklemede.

Uygulama arayüzleri için ek denetim başlatıldı: tamamlanan her kohort videosunun
ilk karesi public CPU ve hibrit Vulkan komutlarıyla ayrı ayrı çalıştırılıyor.
İlk video (Bonfire720p2.6M) iki arayüzde de birebir geçti; hibritte dört Vulkan
matris dispatch'i ve raporlanmış validation hatası yok. Sürüm kopyaları, hash'ler
ve sonuçlar `validation/live_hdr_public_backends_20261102_run01` içinde.
Bu temsilî kare denetimidir; tam video GPU doğrulaması veya yeni benzersiz
örnek sayısı değildir. Ana 4k15M Bonfire testinde şu an 12/18 kare geçti;
tamamlanan kohort kapsamı henüz 10/27 video / 221 toplam benzersiz karede kaldı.

**Son durum:** football9M **17/17** karede Q/Q_JOD bit eşitliğiyle tamamlandı.
Video ortalaması **8.9048662185668945**, hex **410e7a55**; bağımsız Octave
kontrolü geçti. Kohort **10/27 video, 189 kare**. Daylight1 dahil toplam
**12 video çifti, 4 içerik, 221 farklı HDR kare çifti** tekrar denetlendi:
`validation/live_hdr_evidence_20261102_run07`. Yeni `4k_15M_Bonfire` testinde
527 karenin zaman eşleşmesi geçti; 18 seçili kare işleniyor. Korelasyon
denetleyicisi tam kohortun bitmesini bekliyor. Henüz tam bitiş iddiası yok.

Uygulama arayüzü ek kontrolü: CargoBoat9M kare0, `IQM-hdrvdp3-cpu.exe` kullanıcıya
açık adlandırılmış seçeneklerle çalıştırıldı. Q **6.786186695098877**
(**40d92871**), Q_JOD **7.6794109344482422** (**40f5bdbc**) resmî Single
referansla birebir aynı. CPU backend'i, sıfır GPU dispatch'i ve model/FFT
ayarları doğrulandı. Bu, eski deneysel binary'den ayrı bir arayüz kontrolüdür;
tekrar edilen kare toplam 204 örneğe eklenmedi. Kayıt:
`validation/live_hdr_public_cpu_cargoboat9M_20261102_run01`.

**Son kontrol:** kohort **9/27 video, 172 kare** tamamlandı. Daylight1 dahil
toplam **11 video çifti, 4 içerik ve 204 farklı HDR kare çiftinde** Q/Q_JOD
bit eşitliği ve video ortalamaları yeniden doğrulandı. Kayıt:
`validation/live_hdr_evidence_20261102_run06`. `1080p_9M_football1` işleniyor.
Ek tamamlanan ortalamalar: CargoBoat6M **7.2904024124145508**, football6M
**8.8060951232910156**, Bonfire9M **8.3304967880249023**, CargoBoat9M
**7.5433168411254883**; tümünde bağımsız Octave ortalaması aynı.

Mevcut çalışan sürecin sonunda korelasyon denetimi için ayrı izleyici bağlandı;
kohort hash'i/kimlikleri kontrolünden sonra bağımsız Octave doğrulaması yapacak.
Pozitif mühendislik örneği geçti, farklı kohort satırı reddedildi. Gerçek 27-video
korelasyonu henüz hazır değil. Sayısal eşdeğerlik ile özgün makale deneyinin
tekrarını ayıran kriterler `HDRVDP3_REPRODUCTION_ACCEPTANCE.md` içinde kaydedildi.

**Güncel:** `1080p_6M_Bonfire` 18/18 karede birebir geçti. Video Q_JOD
**8.1062984466552734**, hex **4101b366**; Octave ortalaması doğrulandı.
Kohort **5/27 video, 93 kare**; Daylight1 testleriyle toplam **7 video çifti,
4 içerik, 125 farklı HDR kare çifti**. Dosya ve ortalama denetimi:
`validation/live_hdr_evidence_20261102_run05`. `1080p_6M_CargoBoat` işleniyor.

Son korelasyonlar için bağımsız Octave denetçisi hazırlandı. Test vektörlerinde
uyum kontrolü geçti, değiştirilmiş SROCC reddedildi. Gerçek kohort henüz tam
olmadığı için bu test sayıları veri kümesi başarımı olarak raporlanmadı.

Gerçek veriyle devam testi de geçti: football1'in 17 karesi yeni bir klasörde
yeniden çözüldü, hesaplama/girdi hash'leri doğrulandı ve önceki birebir sonuçlar
17/17 karede kullanıldı. Octave ile video ortalaması yine **40f94a91**.
Eski çalışma kesilmedi veya değiştirilmedi; bu bir devam yolu provasıdır,
işletim sistemi çökme testi değildir. Tekrar edilen kareler toplam 107 örneğe
eklenmedi. Kayıt: `validation/live_hdr_football_recovery_20261102_run01`.
Ana kohort değerlendirmesi sıradaki `1080p_6M_Bonfire` ile sürüyor.

**En son:** `1080p_1M_football1` tam video testi geçti: 506 kare zaman eşleşmesi,
17/17 seçili karede Q/Q_JOD bit eşitliği ve bağımsız Octave ortalaması.
Video Q_JOD **7.7903523445129395**, hex **40f94a91**. Kohort **4/27 video,
75 kare**; Daylight1 testleriyle toplam **6 video çifti, 4 içerik, 107 farklı
HDR kare çifti**. Yeniden dosya/ortalama denetimi:
`validation/live_hdr_evidence_20261102_run04/evidence.json`.

football1 kare0 hibrit Vulkan uygulamasında da birebir geçti: Q
**4.5036582946777344**, Q_JOD **5.384850025177002**; dört gerçek Vulkan
matris dispatch'i, raporlanmış validation hatası yok. Hibrit kapsam artık
beş kare çifti / dört içerik. Bu tekrarlar 107 sayısına tekrar eklenmedi.

**Son durum:** CargoBoat1080p1M tam video testi tamamlandı. 650 karenin zaman
eşleşmesi ve 22 seçili karenin Q/Q_JOD bit eşitliği geçti. Video ortalaması
**5.7304935455322266**, Single hex **40b76034**; bağımsız Octave ortalaması da aynı.
Kohort **3/27 video, 58 kare**; iki Daylight1 testiyle toplam doğrulanmış kapsam
**5 video çifti, 3 içerik, 90 farklı HDR kare çifti**. Tüm dosya/ortalama denetimi
`validation/live_hdr_evidence_20261102_run03/evidence.json` içinde; 90 farklı
PNG hash çifti ayrıca doğrulandı. Sıradaki `1080p_1M_football1` otomatik başladı.
Henüz 27 videonun tamamına ait korelasyon veya tam GPU eşdeğerliği iddiası yok.

Aşağıdaki kısmi sayımlar gün içindeki önceki kontrol noktalarıdır.

Ek denetim: `summarize_live_hdr_evidence.cjs`, tamamlanmış dört video için kaynak
MP4/PNG hash'lerini, zaman eşleşmesini, kare skorlarını ve ortalamaları yeniden
kontrol etti. Hem video/kare kimliğine göre hem PNG hash çiftlerine göre sayı
**68**. Aynı test klasörünü tekrar eklemek sayıyı artırmadı. Kayıtlar:
`validation/live_hdr_evidence_20261102_run01` ve `run02`.

CargoBoat1080p1M kare0, hibrit uygulamada da bit düzeyinde geçti: Q
**1.6976785659790039**, Q_JOD **2.171440601348877**, dört Vulkan dispatch'i,
raporlanmış validation hatası yok. Hibrit doğrulama dört kare çifti / üç içerikte
geçti; kalan aşamalar CPU'da. CargoBoat tam video değerlendirmesinde şu an
**14/22** kare eşleşti; tam video henüz kabul edilmedi.

`1080p_1M_Bonfire` / `4k_ref_Bonfire` için 527 karenin zaman eşleşmesi ve
18 seçili karenin Q/Q_JOD bit eşitliği geçti. Video ortalaması
**5.0774068832397461**, Single hex **40a27a1e**; bağımsız Octave ortalaması da aynı.
Tamamlanan video testlerinin toplam kapsamı artık **4 video çifti / 2 içerik /
68 farklı HDR kare çifti**. Aday kohortta 2/27 video tamamlandı.

Bu videonun kare0 örneği public hibrit uygulamada da birebir eşleşti:
Q **5.6833958625793457**, Q_JOD **6.6135272979736328**; dört gerçek Vulkan
matris dispatch'i ve raporlanmış validation hatası yok. Hibrit tekrar, yeni
benzersiz veri örneği sayılmadı. Tam GPU uygulaması henüz tamamlanmadı.

CargoBoat1080p1M değerlendirmesi sürüyor: 650 kare zaman eşleşmesi geçti,
22 seçili karenin 12'si şu ana kadar birebir aynı. Henüz tam video sonucu değil.
Çalışan devam süreci kalan kohortu sırayla değerlendirip tamamlanınca analizi
başlatacak. Anlık durum `tests/status_live_hdr_cohort.cjs` ile okunabilir.

## Önceki tam Bonfire video doğrulaması

`720p_2.6M_Bonfire` / `4k_ref_Bonfire` çiftinde 527 karenin sayıları ve zaman
damgaları eşleştirildi. Sıfır tabanlı 0:30:510 aralığındaki 18 örneğin tamamında
Q ve Q_JOD resmî v3.0.7 CPU Octave referansıyla bit düzeyinde eşleşti.

- Video ortalaması Q_JOD: **7.7758626937866211**.
- Single hex: **40f8d3de**; resmî uygulama ve C++ aynı.
- Sıralı Single ortalama ayrıca Octave ile doğrulandı.
- Kaynak/çıktı hash'leri ve subjektif CSV eşlemesi doğrulandı.
- Kayıt: `validation/live_hdr_bonfire720_video_20261102_run01`.

Önceki iki Daylight1 video testiyle toplam kapsam **üç video çifti, iki içerik,
50 benzersiz gerçek HDR kare çifti** oldu. Bonfire kare0 önce de test edilmişti;
bugün eklenen benzersiz örnek sayısı 17'dir. Hibrit tekrarlar bu sayıya eklenmedi.

## Gerçek hibrit değerlendirme

`IQM-hdrvdp3-hybrid`, Native RGB ve LMSR matris işlemlerini Vulkan üzerinde,
FFT/adaptasyon/JND/pyramid/kalite işlemlerini CPU üzerinde çalıştırıyor. Model
katsayılarının hazırlanması CPU'da kalıyor; sessiz CPU fallback yok.

| Hibrit test | Q, referans = hibrit | Q_JOD, referans = hibrit |
|---|---|---|
| Daylight1 4k_3M kare30 | 7.4749040603637695 | 8.2962665557861328 |
| Bonfire 720p_2.6M kare0 | 7.088564395904541 | 7.9553570747375488 |

Her çalıştırmada dört gerçek Vulkan matris dispatch'i doğrulandı; validation
hatası raporlanmadı. Kaynak PNG'den son skora kadar hesaplama bağımsız C++
sürecinde gerçekleşti; resmî ara değerler hesaplamaya enjekte edilmedi.
CPU'nun yedi CLI kontrolü tekrar geçti. Üç ek kontrol, eksik/geçersiz GPU shader
ile hibrit aracın hata verdiğini ve CPU aracının GPU seçeneğini reddettiğini
doğruladı. Tam GPU veya hızlanma iddiası yoktur.

## Kohort ve istatistik hazırlığı

27 bozulmuş video ve üç referansın oluşturduğu aday yerel alt kümenin 30 MP4'ü
ZIP girişleriyle SHA256 karşılaştırılarak hazırlandı. Dosyalar metadata boyutu
ve container kare sayısı bakımından eşleşiyor; hepsi 10-bit PQ/BT.2020,
BT.2020 non-constant-luminance matris ve limited-range etiketi taşıyor.
Bu, tüm karelerin çözüldüğü veya 27 videonun kalite hesabının bittiği anlamına
gelmiyor. Özgün yarışma train CSV'siyle kohort eşdeğerliği hâlâ doğrulanmadı.

Tamamlanmış video tahminlerini gerektiren yerel SROCC/ham Pearson aracı eklendi.
Eksik kohortu, yanlış etiket/protokol sürümünü ve uyumsuz Single ortalamayı
reddediyor. Mühendislik testleri Octave ile 1e-14 toleransta kontrol edildi;
eşit sıralamalar, geçersiz değerler ve CLI veri birleştirmesi sınandı.
Bu test vektörleri LIVE HDR doğruluk sonuçları değildir. Gerçek 27-video
korelasyonu henüz hesaplanmadı; RMSE için belgelenmemiş bir JOD–MOS eşlemesi
veya yeniden kalibrasyon uygulanmadı.

## Kanıt ve sonraki adım

- [Hibrit kapsamı ve komutlar](HYBRID_SINGLE_QUALITY_VALIDATION.md).
- [Kohort girdi/analiz hazırlığı](LIVE_HDR_COHORT_READINESS.md).
- Video: `validation/live_hdr_bonfire720_video_20261102_run01`.
- Hibrit: `validation/live_hdr_hybrid_frame30_20261102_run01` ve
  `validation/live_hdr_hybrid_bonfire720_20261102_run01`.
- Kaynak girdi denetimi: `validation/live_hdr_cohort_inputs_20261102_run01`.
- İstatistik aracı testleri: `validation/live_hdr_statistics_20261102_run03`.

Sonraki öncelik, ilan edilmiş kohortun diğer tam video tahminlerini tamamlamak,
ardından video MOS değerleriyle korelasyon hesaplamaktır. Genel GPU FFT ve kalan
işleme aşamaları ayrıca tamamlanmalıdır. RGB48 kod çözümü ve başlangıç fazı0
yerel tercihler olarak belgelenmiştir; orijinal yarışma sonuçlarıyla tam tekrar
iddiası yapılmıyor. Yeni değişiklikler henüz commit/push edilmedi.
