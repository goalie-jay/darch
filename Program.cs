using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.IO;

namespace Darch
{
    class Program
    {
    	public static bool Verbose = false;

    	enum OperationMode
    	{
    		EXTRACT,
    		ARCHIVE,
    		UNDEFINED
    	}

    	public static void PrintUsage()
    	{
    		Console.WriteLine("Usage: darch -x/-a [-v] <file/dir1> <file/dir2> ... -o <output>.");
    	}

    	public static int Main(string[] args)
    	{
    		if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
    		{
    			Console.WriteLine("This program is not supported on non-POSIX platforms.");
    			return 77;
    		}

    		OperationMode mode = OperationMode.UNDEFINED;
    		string output = null;
    		List<string> inputObjects = new List<string>();

    		for (int i = 0; i < args.Length; ++i)
    		{
    			string current = args[i];
    			if (current == "-o")
    			{
    				++i;
    				if (i >= args.Length)
    				{
    					PrintUsage();
    					return 1;
    				}

    				output = args[i];
    			}
    			else if (current == "-v")
    				Verbose = true;
    			else if (current == "-x")
    				mode = OperationMode.EXTRACT;
    			else if (current == "-a")
    				mode = OperationMode.ARCHIVE;
    			else
    				inputObjects.Add(current);
    		}

    		if (output == null)
    		{
    			PrintUsage();
    			return 1;
    		}

    		if (inputObjects.Count < 1)
    		{
    			PrintUsage();
    			return 2;
    		}

    		if (mode == OperationMode.UNDEFINED)
    		{
    			PrintUsage();
    			return 3;
    		}

    		if (mode == OperationMode.EXTRACT && inputObjects.Count != 1)
    		{
    			Console.WriteLine("Exactly one input file is allowed for extraction.");
    			return 4;
    		}

    		switch (mode)
    		{
    			case OperationMode.ARCHIVE:
    			{
    				FileStream fileStream = null;

    				try
    				{
    					fileStream = File.Create(output);
    				}
    				catch
    				{
    					Console.WriteLine("Could not open output file for writing.");
    					return 5;
    				}

    				try
    				{
    					Archiver.Archive(inputObjects.ToArray(), fileStream);
    				}
    				catch (Exception ex)
    				{
    					fileStream.Dispose();
    					Console.WriteLine(ex.Message);
    					return 6;
    				}

    				fileStream.Dispose();
    				break;
    			}
    			case OperationMode.EXTRACT:
    			{
    				FileStream fileStream = null;

    				try
    				{
    					fileStream = File.OpenRead(inputObjects[0]);
    				}
    				catch
    				{
    					Console.WriteLine("Could not open input file for reading.");
    					return 8;
    				}

    				try
    				{
    					Archiver.Extract(fileStream, output);
    				}
    				catch (Exception ex)
    				{
    					fileStream.Dispose();
    					Console.WriteLine(ex.Message);
    					return 9;
    				}

    				fileStream.Dispose();
    				break;
    			}

    			default:
    				throw new NotImplementedException();
    		}

    		return 0;
    	}
    }
}