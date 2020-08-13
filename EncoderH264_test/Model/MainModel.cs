using System;
using System.Collections.Generic;
using System.Text;

namespace EncoderH264_test.Model
	{
	public class MainModel
		{
		public Microsoft.Win32.RegistryKey appKey;

		public string lastBinFile { get; set; }
		public string ffmpegCommandLine { get; set; }

		public string lastImagenDeEntrada { get; set; }
		public string lastImagenDeSalida { get; set; }

		public MainModel(Microsoft.Win32.RegistryKey _appKey)
			{
			appKey = _appKey;
			Load();
			}

		public bool Load()
			{
			try
				{
				lastBinFile = appKey.GetValue("lastBinFile", @"d:\").ToString();
				ffmpegCommandLine = appKey.GetValue("ffmpegCommandLine", @"d:\").ToString();

				lastImagenDeEntrada = appKey.GetValue("lastImagenDeEntrada", @"d:\").ToString();
				lastImagenDeSalida = appKey.GetValue("lastImagenDeSalida", @"d:\").ToString();
				}
			catch (Exception ex)
				{
				return false;
				}

			return true;
			}

		public bool Save()
			{
			try
				{
				appKey.SetValue("lastBinFile", lastBinFile);
				appKey.SetValue("ffmpegCommandLine", ffmpegCommandLine);

				appKey.SetValue("lastImagenDeEntrada", lastImagenDeEntrada);
				appKey.SetValue("lastImagenDeSalida", lastImagenDeSalida);
				}
			catch (Exception ex)
				{
				return false;
				}

			return true;
			}
		}
	}
