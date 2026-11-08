# LIVE HDR container-clock diagnostic - 2026-11-07

The full archive run02 stopped after23complete videos/405frames at
1080p_1M_Light2 before anyquality evaluation: strict absolute timestamp equality
failed. Oldfailure/root/results remainpreserved; no numericalmismatch was hidden.

Bothpaired videos decode352frames anddeclare60000/1001fps. Reference4k_ref_Light2
startsPTS16200/timebase1/90000 (0.18seconds); distortedvideo startsPTS0/timebase
1/60000. Relative decimal timestamps differ byatmost6microseconds fromdifferent
clockquantization. Originaldiagnostic:validation/live_hdr_light2_alignment_20261107_run01.

New explicit local policy:selectthe samezero-basedframe index, requiringexact
integerCFRclockconsistency onboth streams aftersubtracting eachfirstPTS. Validate
allframes againstafixedfloor/ceil/nearest-even quantizationrule usingBigInt/rational
arithmetic, identicalrationalframe rates andmatchingmetadata/header/decodedcounts.
No floatingtime tolerance andno qualityscore tolerance areintroduced. StartPTS
mustmatchthefirstdecodedframe. Clockrule isrecorded inplan/alignment.json andraw
ffprobe JSON soindependent audit can recompute it. This doesnotindependentlyprove
pixel-content alignment ororiginalauthor's executeddecoder settings.

Engineeringchecks acceptdifferent origins/timebases onexactgrids andrejectmissing
frames,wholeframe displacement,one-tick jitter andinconsistentframe rates.
Actual352frameLight2clockcertificate passed:referencefloor,testexactgrid.

Oldrunner/scorer/audit files unchanged. Newclockgridrunner,audit,fullrun03controller
andwatcher areseparate files. Light2full12samplepairpilot mustfinishandpass its
independent audit beforefull279continuation proceeds;23oldcompletedvideos are
re-audited/referenced,neveroverwritten. Newroot:
validation/live_hdr_light2_clockgrid_20261107_run01 and
validation/live_hdr_full_scores_20261107_run03. Completequalityacceptance ispending.
