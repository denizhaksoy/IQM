# HDR-VDP-3 — 4 Kasım 2026

## Doğrulanan bulgular

- Önceki sekiz çalışmalık temiz referans deneyi tamamlandı: sekizinde de
  Q `40ec7071`, Q_JOD `41038aea`; C++ ile aynı. Referans ağacı, skor wrapper'ı,
  Octave ve FFTW binary hash'leri deney boyunca değişmemiş.
- İlk başarısız kayıttaki Q `40ec4f50` / Q_JOD `41037c75` açıklanmış değil.
  Bu kayıt korunuyor ve eşleşen tekrarlarla değiştirilmiyor.
- Eski otomatik kohort işi hata nedeniyle durmuş. Kabul edilen kapsam
  **21/27 video, 400 seçili kare**; tam kohort korelasyonu henüz yok.

## Değişiklikler ve kontroller

- Her kare için birden fazla bağımsız referans işlemi çalıştıran tanı modu
  eklendi. Bütün Q/Q_JOD bitleri aynı olmalı ve C++ ile eşleşmeli.
- Çoğunluk oyu yok: eski farklı skorun iki eşleşen skor arasında olduğu test
  özellikle başarısız kabul edildi. Girdi/ayar farkı, geçersiz bitler ve sonlu
  olmayan skorlar da reddedildi. **Dokuz kontrol geçti**.
- Tekrarlı referans dosyalarının hash'leri saklanıyor ve son denetimde yeniden
  doğrulanıyor. Bu mod önceki kare sonuçlarını yeniden kullanmıyor.
- İşleme önceliği verilebiliyor; örneklenen kare listesi ve video ortalamasının
  sırası değişmiyor. Önce şüpheli kare150 işleniyor.

## Çalışan tanı

Yeni çıktı: `validation/live_hdr_football_triplicate_20261104_run01`.
`540p_2.2M_football1` videosunun 17 seçili karesinin her biri için üç temiz
referans çalışması ve bağımsız C++ sonucu isteniyor. Ayrı postprocess işi,
tam video tamamlandığında tekrar kayıtlarını, girdi hash'lerini ve Octave ile
video ortalamasını doğrulayacak. Sonuç eski kohorta otomatik eklenmeyecek.

İlk kontrol: kare150 ve kare0 üçer bağımsız referans çalışmasında ve C++
hesabında aynı bitlerle geçti. Kare150 Q **40ec7071**, Q_JOD **41038aea**;
üç skor dosyasının SHA256 kayıtları saklandı. Kalan 15 kare işleniyor.

## Koşullu devam zinciri

Güncel üçlü video denetimi **8/17** karede geçti; kalan kareler hesaplanıyor.
Ek küçük deneylerde, kaydedilmiş taban bandının32ofset FFT sonucu aynı spektrum
ve aynı frekansı verdi. Farklı fiziksel frekans tepesine fark yaklaşık%10.297.
Kalite havuzlamasında da6bandın8ofset deneyi aynı hata bitlerini ve aynı Q/JOD'u
verdi. Bu sonuçlar yalnızca sonraki eşleşen çalışmanın saklanmış ara değerleri
için geçerli; ilk farklı çalışmanın nedeni kanıtlanmış değil.

Son kontrol: kare0/30/60/150, üçer referans süreci ve C++ ile birebir geçti
(4/17). Tam video tamamlanınca bağımsız ortalama ve tüm kayıt hash'leri
denetlenecek. Bu koşullar geçerse yeni bir çalışma klasöründe eski21 doğrulanmış
video ve bu yeni tekrar kullanılacak; kalan beş video için üçlü referans
koşulu uygulanacak. Eski başarısız kohort ve ilk farklı skor değiştirilmeyecek.

Yeni devam kökü `validation/live_hdr_verified_continuation_20261104_run01`.
Durum `continuation_status.json` içinde. Zincir, tamamlanan yerel kohort için
korelasyonları, bağımsız Octave denetimini, tüm kanıt kayıtlarını ve temsilî
karelerde public CPU/hibrit denetimini çalıştıracak. İlk tarihsel anomalinin
açık olduğu bilgisi tahmin manifestine ve analiz raporuna taşınacak. Eski21
videonun tek referanslı kontrolleri üçlü yapılmış gibi gösterilmeyecek; her
satırda kullanılan referans tekrar sayısı belirtilecek.

İstatistik regresyon kontrolleri ve anomali bilgisinin raporda korunması geçti.
Başarısız bir tanı çalışmasının devam zincirine alınması da testte reddedildi;
yeni değerlendirme ayarı dahi oluşturulmadı. Yeni sayısal koşul gevşetilmedi:
üç referansın tümü aynı olmalı, ardından C++ bitleri de aynı olmalı.

Bu, referans tutarlılığı deneyi. İlk farkın kök nedeni henüz kanıtlanmadığı için
metrik hatası tamamen düzeltildi, özgün makale sonucu tekrarlandı veya her
koşulda yüzde100 eşdeğerlik sağlandı iddiası yapılmıyor.
