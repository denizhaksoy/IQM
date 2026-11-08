# Original challenge scope - 2026-11-07

Primary source: https://sites.google.com/view/wacv2023-workshop-quality-va/competition

The 2023 competition page distinguishes:
- Dataset description:27 UHD sources,9 encodings each,243 distorted sequences.
- Public leaderboard:27 FR predictions after excluding Bonfire,CargoBoat,football1 reference videos (30 NR entries).
- Final ranking:separate reserved/private videos, evaluated through submitted runnable models. Public leaderboard is not final ranking.
- Released labels:bright-condition Sureal MOS in train_video_info.csv.

These are distinct scopes. Our full supplied archive31sources/279distortions cannot be assumed identical to the243description or private final set. Our prior27video local candidate resembles the public leaderboard content grouping; exact ID equality still needs original CSV evidence. No silent removal of4contents or inferred243subset.

The original template URL https://github.com/Easoncyx/2023_wacv_vqm_challenge now redirects to https://github.com/Easoncyx/vqm_challenge . Its current README describes L40S hardware, unlike the2023 page's p3.2xlarge. Current repository instructions must not automatically be attributed to2023; historical revisions need inspection.

Next: obtain original train_video_info.csv and dated submission-template revision; distinguish public leaderboard reproduction from unavailable private-final reproduction. No author contact has been sent. Existing279numerical comparison continues as separate implementation evidence.

Historical template retrieved and verified:commit0c8ce3ef1d69240d3f43b2cf1061a47dd83e9b5f (2022-11-29). Its12 tree entries contain no original training CSV or author predictions. Artifact:validation/hdrvdp3_original_protocol_20261107_run01. Machine-readable protocol audit passed the16frame local reconstruction while correctly retaining original_paper_reproduction_verified=false:validation/hdrvdp3_paper_protocol_acceptance_20261107_run01/report.json. Original final-set equivalence requires access to its inputs and numerical targets; more runs on the public archive cannot substitute for these artifacts.
