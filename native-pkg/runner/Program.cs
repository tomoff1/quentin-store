// GPL-3.0-or-later. QUENTIN STORE native PS5 package builder and static verifier.
using System.Security.Cryptography;
using System.Buffers.Binary;
using System.Text.Json;
using LibProsperoPkg;
using LibProsperoPkg.Content;
using LibProsperoPkg.PKG;

if (args.Length != 3) throw new ArgumentException("Usage: Runner <native-app-folder> <candidate.pkg> <validation-folder>");
string appRoot = Path.GetFullPath(args[0]);
string finalPath = Path.GetFullPath(args[1]);
string validationDir = Path.GetFullPath(args[2]);
Directory.CreateDirectory(validationDir);
string extractedRoot = Path.Combine(validationDir, "extracted-" + Guid.NewGuid().ToString("N"));
using var paramDoc = JsonDocument.Parse(File.ReadAllBytes(Path.Combine(appRoot, "sce_sys/param.json")));
string cid = paramDoc.RootElement.GetProperty("contentId").GetString()!;
string tid = paramDoc.RootElement.GetProperty("titleId").GetString()!;
string version = paramDoc.RootElement.GetProperty("masterVersion").GetString()!;
var sourceHashes = Directory.EnumerateFiles(appRoot, "*", SearchOption.AllDirectories)
    .ToDictionary(f => Path.GetRelativePath(appRoot, f).Replace('\\', '/'), f => SHA256.HashData(File.ReadAllBytes(f)));
if (sourceHashes.ContainsKey("sce_sys/param.sfo")) throw new InvalidDataException("PS4 metadata is forbidden in this PS5 package.");
if (sourceHashes.ContainsKey("sce_sys/about/right.sprx")) throw new InvalidDataException("Unreviewed right.sprx is forbidden in this build.");
var before = ProsperoLaunchReadiness.InspectAppRoot(appRoot);
if (!before.IsLaunchReady) throw new InvalidDataException("Source module checks: " + string.Join("; ", before.Issues));
Directory.CreateDirectory(Path.GetDirectoryName(finalPath)!);
var result = ProsperoPackageBuilder.Build(new ProsperoBuildOptions {
    Mode = ProsperoPackageMode.Application,
    OutputFormat = ProsperoOutputFormat.DebugImage,
    SourceFolder = appRoot,
    OutputFolder = Path.GetDirectoryName(finalPath)!,
    ContentId = cid,
    TitleId = tid,
    Title = "QUENTIN STORE PS5",
    Version = version,
    GenerateParamJsonIfMissing = false,
    LicenseFree = true
}, Console.WriteLine);
if (!result.LicenseFree || result.DebugLicense?.RequiresRif != false)
    throw new InvalidDataException("Expected license-free debug output.");
if (!string.Equals(Path.GetFullPath(result.OutputPath), finalPath, StringComparison.OrdinalIgnoreCase))
    File.Copy(result.OutputPath, finalPath, true);
foreach (var pair in sourceHashes)
    if (!SHA256.HashData(File.ReadAllBytes(Path.Combine(appRoot, pair.Key))).SequenceEqual(pair.Value))
        throw new InvalidDataException("Builder changed source file: " + pair.Key);
var package = ProsperoPkgReader.Read(finalPath);
var accepted = ProsperoPkgValidator.Validate(package, cid);
foreach (var check in accepted.Checks) Console.WriteLine($"{check.Status}: {check.Name}: {check.Detail}");
if (!accepted.Accepted || package.Fih is null || package.Fih.IsOfficial || package.Fih.FormatVersion != 3)
    throw new InvalidDataException("Package structural checks failed.");
if (package.Header?.ContentType != 0x20) throw new InvalidDataException("Expected PS5 base-application content type 0x20.");
var inspect = ProsperoPackageExtractor.Inspect(finalPath);
if (inspect.RequiresSuppliedKey || inspect.IsRetail || inspect.ContentId != cid)
    throw new InvalidDataException("Package extraction preconditions failed.");
var manifest = ProsperoPackageExtractor.Extract(finalPath, extractedRoot, new string('0', 32), Console.WriteLine);
string actualApp = File.Exists(Path.Combine(extractedRoot, "eboot.bin")) ? extractedRoot : Path.Combine(extractedRoot, "uroot");
var verified = new List<object>();
foreach (var pair in sourceHashes) {
    string path = Path.Combine(actualApp, pair.Key);
    byte[] bytes;
    if (File.Exists(path)) bytes = File.ReadAllBytes(path);
    else {
        string entryName = pair.Key.StartsWith("sce_sys/", StringComparison.Ordinal) ? pair.Key[8..] : pair.Key;
        var entry = package.Entries.FirstOrDefault(e => e.Name == entryName || (pair.Key == "sce_sys/param.json" && e.RawId == 0x2000));
        if (entry is null || entry.Encrypted) throw new InvalidDataException("Source file missing from package: " + pair.Key);
        using var stream = File.OpenRead(finalPath);
        long offset = checked((long)package.Fih.EmbeddedCntOffset + entry.DataOffset);
        if (offset < 0 || entry.DataSize > stream.Length - offset) throw new InvalidDataException("Metadata entry out of bounds.");
        stream.Position = offset;
        bytes = new byte[entry.DataSize];
        stream.ReadExactly(bytes);
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        File.WriteAllBytes(path, bytes);
    }
    if (!SHA256.HashData(bytes).SequenceEqual(pair.Value))
        throw new InvalidDataException("Round-trip SHA-256 mismatch: " + pair.Key);
    verified.Add(new { path = pair.Key, size = bytes.Length, sha256 = Convert.ToHexString(pair.Value).ToLowerInvariant() });
}
if (Directory.EnumerateFiles(extractedRoot, "right.sprx", SearchOption.AllDirectories).Any())
    throw new InvalidDataException("Packager injected an unreviewed rights module.");
var after = ProsperoLaunchReadiness.InspectAppRoot(actualApp);
if (!after.IsLaunchReady) throw new InvalidDataException("Extracted module checks: " + string.Join("; ", after.Issues));
var nativeAbiChecks = new List<object>();
foreach (string relative in new[] { "eboot.bin", "sce_module/libc.prx" }) {
    byte[] bytes = File.ReadAllBytes(Path.Combine(actualApp, relative));
    SelfImage module = ProsperoFself.Parse(bytes);
    ReadOnlySpan<byte> elf = module.Elf;
    ushort expectedType = relative == "eboot.bin" ? (ushort)0xFE10 : (ushort)0xFE18;
    if (elf.Length < 64 || !elf[..4].SequenceEqual(new byte[] { 0x7f, 0x45, 0x4c, 0x46 }) || elf[4] != 2 || elf[5] != 1 || elf[7] != 9
        || BinaryPrimitives.ReadUInt16LittleEndian(elf[16..]) != expectedType
        || BinaryPrimitives.ReadUInt16LittleEndian(elf[18..]) != 62)
        throw new InvalidDataException("Native PS5 module ABI header invalid: " + relative);
    if (module.Segments.Count == 0 || module.Segments.Any(s => s.Encrypted || s.FileOffset > (ulong)bytes.Length || s.FileSize > (ulong)bytes.Length - s.FileOffset))
        throw new InvalidDataException("Native module segment bounds/encryption invalid: " + relative);
    nativeAbiChecks.Add(new { path = relative, elfType = $"0x{expectedType:X4}", machine = "x86_64", elfClass = "ELF64", osAbi = "FreeBSD", plaintextSelf = true });
}
var report = new {
    package = finalPath,
    size = new FileInfo(finalPath).Length,
    sha256 = Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(finalPath))).ToLowerInvariant(),
    contentId = cid, titleId = tid, contentVersion = paramDoc.RootElement.GetProperty("contentVersion").GetString(),
    licenseFree = true, sourceRestored = true, roundTripVerified = verified,
    structuralChecks = accepted.Checks,
    extractedModuleChecks = after.Modules,
    nativeAbiChecks,
    unreviewedRightsModulePresent = false,
    physicalPS5ExecutionTested = false,
    limitations = "Static package validation and extraction do not establish console installation or execution compatibility. Embedded upstream right.sprx injection intentionally disabled because provenance is unverified."
};
File.WriteAllText(Path.Combine(validationDir, "validation.json"), JsonSerializer.Serialize(report, new JsonSerializerOptions { WriteIndented = true }));
Console.WriteLine("STATIC VALIDATION PASSED: " + finalPath);
