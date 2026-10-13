// LibProsperoPkg - A library for building and inspecting PS5 packages.
// Copyright (C) 2026 SvenGDK
//
// Per-file policy for the inner image: whether a file is submitted to the codec, stored verbatim, or
// cannot be packaged as it stands. The decision reads only the file's leading bytes and is taken once
// per file; everything below it (per-payload compression, per-block store rules) holds no file-level
// policy of its own.
#nullable enable
using System;
using System.Buffers.Binary;

namespace LibProsperoPkg.PFS;

/// <summary>How the inner image treats one file.</summary>
public enum ProsperoInnerFilePolicy
{
    /// <summary>Submitted to the codec; each block then decides for itself whether to stay compressed.</summary>
    Compress,

    /// <summary>Stored verbatim: every block of the file is a stored block.</summary>
    StoreVerbatim,

    /// <summary>The file is an executable image that must become a signed module before it can be packaged.</summary>
    RequiresModuleConversion,
}

/// <summary>
/// Classifies a file for the inner image. An application or additional-content volume stores only
/// already-signed modules verbatim; an unsigned executable image has to be converted first, except for
/// the one shape that is not an executable at all and is compressed like any other file.
/// </summary>
public static class ProsperoInnerFileClassifier
{
    /// <summary>A signed module container.</summary>
    private const uint SignedModuleMagic = 0xEEF51454;

    /// <summary>A raw executable image.</summary>
    private const uint ExecutableImageMagic = 0x464C457F;

    /// <summary>Offset of the executable image's type field.</summary>
    private const int ImageTypeOffset = 0x10;

    /// <summary>Offset of the executable image's machine field.</summary>
    private const int ImageMachineOffset = 0x12;

    /// <summary>Bytes of the file header the classifier reads at most.</summary>
    public const int HeaderLength = 0x14;

    /// <summary>
    /// Classifies a file from its leading bytes. Passing fewer than <see cref="HeaderLength"/> bytes is
    /// allowed; a file too short to identify is compressed.
    /// </summary>
    /// <param name="header">The file's leading bytes.</param>
    public static ProsperoInnerFilePolicy Classify(ReadOnlySpan<byte> header)
    {
        if (header.Length < 4)
            return ProsperoInnerFilePolicy.Compress;

        uint magic = BinaryPrimitives.ReadUInt32LittleEndian(header);
        if (magic == SignedModuleMagic)
            return ProsperoInnerFilePolicy.StoreVerbatim;

        if (magic == ExecutableImageMagic && header.Length >= HeaderLength)
        {
            ushort imageType = BinaryPrimitives.ReadUInt16LittleEndian(header[ImageTypeOffset..]);
            ushort machine = BinaryPrimitives.ReadUInt16LittleEndian(header[ImageMachineOffset..]);
            // This one shape is data that merely shares the magic, and is compressed like any other file.
            if (imageType == 2 && machine == 0x00E0)
                return ProsperoInnerFilePolicy.Compress;
            return ProsperoInnerFilePolicy.RequiresModuleConversion;
        }

        return ProsperoInnerFilePolicy.Compress;
    }
}
