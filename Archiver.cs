using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using ArchiveInt = System.Int32;

namespace Darch
{
    public struct ArchiveFileEntry
    {
        public ArchiveInt Magic;
        public ArchiveInt RelativePathLength;
        public byte[] RelativePathASCII;
        public ArchiveInt Permissions;
        public ArchiveInt BinaryLength;
        public byte[] BinaryContents;
    }

    public class ArchiveHeader
    {
        public ArchiveInt Magic;
        public ArchiveInt FileCount;
        public List<ArchiveFileEntry> Entries;

        public void AddEntry(ArchiveFileEntry entry)
        {
            Entries.Add(entry);
            ++FileCount;
        }

        public void AddFile(string relativePath, byte[] binaryContents, UnixFileMode permissions)
        {
            ArchiveFileEntry entry = new ArchiveFileEntry();

            entry.Magic = Archiver.HEADER_MAGIC;
            entry.RelativePathASCII = Encoding.ASCII.GetBytes(relativePath.ToCharArray());
            entry.RelativePathLength = entry.RelativePathASCII.Length;
            entry.Permissions = (ArchiveInt)permissions;
            entry.BinaryLength = binaryContents.Length;
            entry.BinaryContents = binaryContents;

            AddEntry(entry);
        }

        public ArchiveHeader()
        {
            Magic = Archiver.HEADER_MAGIC;
            FileCount = 0;
            Entries = new List<ArchiveFileEntry>();
        }
    }

    public static class Archiver
    {
        public const ArchiveInt HEADER_MAGIC = 0x992FF;

        static class StreamHelper
        {
            public static void WriteInt(FileStream stream, ArchiveInt i)
            {
                byte[] binary = BitConverter.GetBytes(i);
                stream.Write(binary, 0, binary.Length);
            }

            public static void WriteByteArr(FileStream stream, byte[] arr)
            {
                stream.Write(arr, 0, arr.Length);
            }

            public static ArchiveInt ReadInt(FileStream stream)
            {
                byte[] binary = new byte[sizeof(ArchiveInt)];
                stream.Read(binary, 0, sizeof(ArchiveInt));

                return BitConverter.ToInt32(binary);
            }

            public static byte[] ReadByteArray(FileStream stream, ArchiveInt len)
            {
                byte[] binary = new byte[len];
                stream.Read(binary, 0, len);

                return binary;
            }
        }

        private static void AddFileToArchive(ArchiveHeader archive, string filename, string parent)
        {
            try
            {
                UnixFileMode mode = File.GetUnixFileMode(filename);
                byte[] binary = File.ReadAllBytes(filename);
                archive.AddFile(Path.Combine(parent, Path.GetFileName(filename)), binary, mode);
            }
            catch
            {
                throw new Exception($"File {filename} could not be read.");
            }
        }

        private static void AddDirectoryRecursiveToArchive(ArchiveHeader archive, string dir, string parent)
        {
            foreach (string file in Directory.GetFiles(dir))
                AddFileToArchive(archive, file, Path.Combine(parent, Path.GetFileName(dir)));

            foreach (string subdir in Directory.GetDirectories(dir))
                AddDirectoryRecursiveToArchive(archive, subdir, Path.Combine(parent, Path.GetFileName(dir)));
        }

        private static void WriteArchiveToFile(ArchiveHeader archive, FileStream stream)
        {
            StreamHelper.WriteInt(stream, archive.Magic);
            StreamHelper.WriteInt(stream, archive.FileCount);

            foreach (ArchiveFileEntry entry in archive.Entries)
            {
                StreamHelper.WriteInt(stream, entry.Magic);
                StreamHelper.WriteInt(stream, entry.RelativePathLength);
                StreamHelper.WriteByteArr(stream, entry.RelativePathASCII);
                StreamHelper.WriteInt(stream, entry.Permissions);
                StreamHelper.WriteInt(stream, entry.BinaryLength);
                StreamHelper.WriteByteArr(stream, entry.BinaryContents);
            }
        }

        public static void Archive(string[] objectNames, FileStream output)
        {
            ArchiveHeader archive = new ArchiveHeader();

            foreach (string obj in objectNames)
            {
                if (File.Exists(obj))
                    AddFileToArchive(archive, obj, ".");
                else if (Directory.Exists(obj))
                    AddDirectoryRecursiveToArchive(archive, obj, ".");
                else
                    throw new Exception($"Object {obj} queued for archiving could not be found.");
            }

            WriteArchiveToFile(archive, output);
        }

        private static void ReconstructArchiveDataFromFile(ArchiveHeader archive, FileStream input)
        {
            ArchiveInt headerMagic = StreamHelper.ReadInt(input);
            if (headerMagic != Archiver.HEADER_MAGIC)
                throw new Exception("Magic value did not match.");

            ArchiveInt fileCount = StreamHelper.ReadInt(input);

            for (ArchiveInt i = 0; i < fileCount; ++i)
            {
                ArchiveFileEntry entry = new ArchiveFileEntry();

                entry.Magic = StreamHelper.ReadInt(input);
                if (entry.Magic != Archiver.HEADER_MAGIC)
                    throw new Exception("Magic value did not match.");

                entry.RelativePathLength = StreamHelper.ReadInt(input);
                entry.RelativePathASCII = StreamHelper.ReadByteArray(input, entry.RelativePathLength);
                entry.Permissions = StreamHelper.ReadInt(input);
                entry.BinaryLength = StreamHelper.ReadInt(input);
                entry.BinaryContents = StreamHelper.ReadByteArray(input, entry.BinaryLength);

                archive.AddEntry(entry);
            }
        }

        public static void Extract(FileStream input, string outputDir)
        {
            ArchiveHeader archive = new ArchiveHeader();

            ReconstructArchiveDataFromFile(archive, input);

            if (!Directory.Exists(outputDir))
                try
                {
                    Directory.CreateDirectory(outputDir);
                }
                catch
                {
                    throw new Exception("Could not create the output directory.");
                }

            try
            {
                foreach (ArchiveFileEntry entry in archive.Entries)
                {
                    string destination = Path.Combine(outputDir, Encoding.ASCII.GetString(entry.RelativePathASCII));

                    if (Program.Verbose)
                        Console.WriteLine(destination);

                    string destinationParent = Path.GetDirectoryName(destination);
                    if (!Directory.Exists(destinationParent))
                        Directory.CreateDirectory(destinationParent);

                    FileStream fileStream = File.Create(destination);
                    fileStream.Write(entry.BinaryContents, 0, entry.BinaryContents.Length);
                    fileStream.Dispose();

                    File.SetUnixFileMode(destination, (UnixFileMode)entry.Permissions);
                }
            }
            catch
            {
                throw new Exception("An error occurred during extraction.");
            }
        }
    }
}