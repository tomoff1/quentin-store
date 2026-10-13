// LibProsperoPkg - A library for building and inspecting PS5 packages.
// Copyright (C) 2026 SvenGDK
//
// ---------------------------------------------------------------------------------------------------
// Generates a valid naps_pkg_layout.dat for a nwonly data-first inner image assembled by
// ProsperoPs5InnerImageAssembler. The run/flush schedule is derived statically from block-aligned
// file starts, the block after each Kraken-compressed file, and dedup padding/metadata re-anchors.
// ---------------------------------------------------------------------------------------------------
#nullable enable
using LibProsperoPkg.PFS;
using LibProsperoPkg.PFS.Compression;
using System;
using System.Collections.Generic;
using System.Linq;

namespace LibProsperoPkg.PKG;

/// <summary>Generates a valid <c>naps_pkg_layout.dat</c> for an assembled nwonly inner image.</summary>
public static class ProsperoNwonlyNapsGenerator
{
    private const int Block64K = 0x10000;
    private const long Ublock256K = 0x40000;

    /// <summary>
    /// Builds the naps bytes for the inner image described by <paramref name="result"/>. The schedule is
    /// derived; pass <paramref name="runOverride"/> to supply explicit on-disk RUN offsets when the
    /// schedule is known.
    /// </summary>
    public static byte[] Generate(ProsperoPs5InnerImageResult result, IReadOnlyCollection<long>? runOverride = null)
    {
        ArgumentNullException.ThrowIfNull(result);
        var placements = result.Placements;
        long mountSize = result.Ndblock * Block64K;
        long metaBase = result.MetaBaseLogical;
        long dataEnd = result.DataEndLogical;

        // ---- DATA-region placements + derivable run schedule ----------------------------------------
        var files = placements.Select(p => new NapsFilePlacement
        {
            OnDiskOffset = p.OnDiskOffset,
            LogicalOffset = p.LogicalOffset,
            OnDiskSize = p.OnDiskSize,
            UncompressedSize = p.UncompressedSize,
            StoreRaw = p.StoreRaw,
            CompressedKde = 2,
        }).ToList();

        HashSet<long> runSet;
        if (runOverride is not null)
        {
            runSet = new HashSet<long>(runOverride);
        }
        else
        {
            // A RUN opens at each block-aligned file start (raw system files) and at the first file after a
            // Kraken-compressed file (the compressed cursor diverges from the on-disk offset there).
            runSet = new HashSet<long>();
            for (int i = 0; i < placements.Count; i++)
            {
                // A RUN opens (re-anchoring the compressed cursor) at EVERY file's on-disk start —
                // whether the file is block-aligned (keystone, right.sprx) or packs mid-block after a
                // previous raw file (e.g. eboot.bin, which begins at right.sprx's on-disk end, not a
                // 64 KiB boundary). A "block-aligned only" rule would drop the mid-block file's RUN.
                runSet.Add(placements[i].OnDiskOffset);

                // The compressed offset space re-bases every 11 ublocks within a run. For a raw file this
                // is on-disk offset + 11*256K, 22*256K, and so on.
                if (placements[i].StoreRaw)
                {
                    long ublocks = (placements[i].UncompressedSize + Ublock256K - 1) / Ublock256K;
                    for (long m = 11; m < ublocks; m += 11)
                        runSet.Add(placements[i].OnDiskOffset + m * Ublock256K);
                }
            }
        }

        // ---- Tail: padding blocks + metadata blocks + terminator ------------------------------------
        var tail = new List<NapsCblockPlanEntry>();

        // Padding covers the logical gap [dataEnd, metaBase) with a SINGLE bare STD cblock
        // (StartRun = false, KdePredictor = 4). The entry decodes to zero-fill, consumes zero
        // forward on-disk cursor progress, and shares the metadata anchor
        // (`BlockInfoOnDiskOffset`) so the outer XTS tweak cursor stays consistent. The
        // metadata Kraken cblock chain emitted below (KdePredictor = 2) covers
        // [metaBase, mountSize) and the naps u2c mapping-entry for `metaBase >> 21` points
        // directly at it — with only one padding entry (well before the metaBase U-block),
        // `first[metaBase / Ublock256K]` resolves to the metadata Kraken cblock on the first
        // probe and the phase-A/B backward walker in `read_naps_pfs_image_start` @
        // k1001+0x5c3f10 is never invoked.
        //
        // `dataBlocks` is padded upstream (ProsperoPs5InnerImageAssembler) so `metaBase` lands
        // on a U-block boundary; combined with the single-STD topology here, the walker returns
        // cleanly and inner-sblock reads at Q = metaBase never fall through to
        // `bread_naps_pfs_image_for_inner`'s `NAPS_MISS_BLKNO` sentinel.
        long paddingStart = dataEnd & ~(Ublock256K - 1);
        tail.Add(new NapsCblockPlanEntry
        {
            StartRun = false,
            OnDiskOffset = result.BlockInfoOnDiskOffset,
            LogicalOffset = paddingStart,
            EvenChunkCompressedLength = 8,
            StreamLength = 0x10,
            Even = 0,
            Odd = 1,
            KdePredictor = 4,
            ShuffleIndex = 0,
        });

        // Metadata blocks: the assembler already captured the compressed metadata's per-256K-block chunk
        // table (ProsperoInnerMetaBlockChunk), so reuse it instead of Kraken-packing the metadata again.
        // Fall back to a fresh pack only if the assembler did not supply the table (e.g. raw metadata).
        IReadOnlyList<ProsperoInnerMetaBlockChunk> metaChunks = result.MetadataBlocks;
        if (metaChunks.Count == 0)
        {
            var metaFile = ProsperoCompressedPfsFile.Parse(
                ProsperoCompressedPfsImage.Pack(result.MetadataPlaintext, 7, (int)Ublock256K));
            metaChunks = metaFile.Blocks.Select(b => new ProsperoInnerMetaBlockChunk(
                b.CompressedSize, b.UncompressedSize, b.IsMultiChunk, b.FirstChunkCompressedSize)).ToList();
        }
        long metaOnDisk = result.MetadataOnDiskOffset;
        bool metaCompressed = result.CompressedMetadata.Length < result.MetadataPlaintext.Length;
        long metaCursor = metaOnDisk;
        int metaCount = metaChunks.Count;
        for (int i = 0; i < metaCount; i++)
        {
            var blk = metaChunks[i];
            int even = blk.IsMultiChunk ? blk.FirstChunkCompressedSize : blk.CompressedSize;
            tail.Add(new NapsCblockPlanEntry
            {
                // The metadata section opens a RUN only on its FIRST block; the remaining metadata chunks
                // continue the compressed cursor under that run (the trailing terminator opens its own RUN).
                // A "first and last" rule would insert a spurious RUN before the final metadata chunk.
                StartRun = i == 0,
                OnDiskOffset = metaCursor,
                LogicalOffset = metaBase + (long)i * Ublock256K,
                EvenChunkCompressedLength = metaCompressed ? even : blk.UncompressedSize,
                StreamLength = blk.CompressedSize,
                Even = 0,
                Odd = 1,
                KdePredictor = (byte)(metaCompressed ? 2 : 4),
                ShuffleIndex = (byte)(metaCompressed && i < metaCount - 1 ? 2 : 0),
            });
            metaCursor += blk.CompressedSize;
        }

        // Terminator marks the mount end.
        tail.Add(new NapsCblockPlanEntry { StartRun = true, OnDiskOffset = metaCursor, LogicalOffset = mountSize, Terminator = true });

        // ---- fidx: afid logical offsets + dataEnd + metaBase + mount --------------------------------
        var fidx = new List<long>(result.AfidLogicalOffsets);
        fidx.Add(dataEnd);
        fidx.Add(metaBase);
        fidx.Add(mountSize);

        int numUBlocks = (int)((mountSize + Ublock256K - 1) / Ublock256K);
        int numOuterBlocks = (result.Image.Length + Block64K - 1) / Block64K;

        var doc = ProsperoNapsLayoutBuilder.BuildFromInnerImage(
            numUBlocks: numUBlocks,
            numOuterBlocks: numOuterBlocks,
            files: files,
            runStartOnDiskOffsets: runSet,
            tailBlocks: tail,
            fileLogicalOffsets: fidx);

        return ProsperoNapsLayout.BuildLayout(doc);
    }
}
