using System;
using System.Collections.Generic;
using System.Text;

namespace EncoderH264_test.ViewModel
	{
	public class MainViewModel : System.ComponentModel.INotifyPropertyChanged
		{
		public static Microsoft.Win32.RegistryKey appByJOChKey;
		static System.Globalization.CultureInfo ci = new System.Globalization.CultureInfo("en-US", false);
		public static MainWindow mainWindows { get; set; }
		public EncoderH264_test.Model.MainModel model;

		#region mvvm
		public event System.ComponentModel.PropertyChangedEventHandler PropertyChanged;

		// This method is called by the Set accessor of each property.
		// The CallerMemberName attribute that is applied to the optional propertyName
		// parameter causes the property name of the caller to be substituted as an argument.
		// private void NotifyPropertyChanged([System.Runtime.CompilerServices.CallerMemberName] String propertyName = "")
		private void NotifyPropertyChanged([System.Runtime.CompilerServices.CallerMemberName] String propertyName = "")
			{
			if (PropertyChanged != null)
				{
				PropertyChanged(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
				}
			}

		private void StoreStringChagedValue(String value, [System.Runtime.CompilerServices.CallerMemberName] String propertyName = "")
			{
			if (!String.IsNullOrWhiteSpace(propertyName))
				{
				appByJOChKey.SetValue(propertyName, value);
				}
			}

		private void StoreIntChagedValue(int value, [System.Runtime.CompilerServices.CallerMemberName] String propertyName = "")
			{
			if (!String.IsNullOrWhiteSpace(propertyName))
				{
				appByJOChKey.SetValue(propertyName, value);
				}
			}

		private void StoreDoubleChagedValue(double value, [System.Runtime.CompilerServices.CallerMemberName] String propertyName = "")
			{
			if (!String.IsNullOrWhiteSpace(propertyName))
				{
				appByJOChKey.SetValue(propertyName, value.ToString(ci));
				}
			}

		private void StoreFloatChagedValue(float value, [System.Runtime.CompilerServices.CallerMemberName] String propertyName = "")
			{
			if (!String.IsNullOrWhiteSpace(propertyName))
				{
				appByJOChKey.SetValue(propertyName, value.ToString(ci));
				}
			}

		private void StoreBoolChagedValue(bool value, [System.Runtime.CompilerServices.CallerMemberName] String propertyName = "")
			{
			if (!String.IsNullOrWhiteSpace(propertyName))
				{
				appByJOChKey.SetValue(propertyName, value);
				}
			}

		#endregion mvvm

		#region properties_generales
		string titulo_v = "EncoderH264 test (h264 4:2:0 / 4:4:4) 1.0 byJOCh";
		public string titulo
			{
			get => titulo_v;
			set { if (titulo_v != value) { titulo_v = value; NotifyPropertyChanged(); } }
			}

		System.Windows.Media.Imaging.BitmapSource imagenDeEntrada_v = null;
		public System.Windows.Media.Imaging.BitmapSource imagenDeEntrada
			{
			get
				{
				return this.imagenDeEntrada_v;
				}

			set
				{
				this.imagenDeEntrada_v = value;
				this.NotifyPropertyChanged();
				}
			}

		System.Windows.Media.Imaging.BitmapSource imagenDeSalida_v = null;
		public System.Windows.Media.Imaging.BitmapSource imagenDeSalida
			{
			get
				{
				return this.imagenDeSalida_v;
				}

			set
				{
				this.imagenDeSalida_v = value;
				this.NotifyPropertyChanged();
				}
			}

		#endregion properties_generales

		public MainViewModel()
			{
			model = new EncoderH264_test.Model.MainModel(appByJOChKey);

			AppCloseCommand = new Extras.RelayCommand(CloseMainWindows, param => this.CanCloseWindows);
			AppMaximizeCommand = new Extras.RelayCommand(MaximizeMainWindows, param => this.CanCloseWindows);
			AppMinimizeCommand = new Extras.RelayCommand(MinimizeMainWindows, param => this.CanCloseWindows);

			saveCommand = new Extras.RelayCommand(Save, param => this.configChange);
			reloadCommand = new Extras.RelayCommand(Reload, param => this.originalChange);

			startServerCommand = new Extras.RelayCommand(StartServer, param => this.CanStartServer);
			stopServerCommand = new Extras.RelayCommand(StopServer, param => this.CanStopServer);

			configTextChangeCommand = new Extras.RelayCommand(ConfigTextChange, param => this.canConfigTextChange);

			selectBinFile0Command = new Extras.RelayCommand(SelectBinFile0, param => this.canOpenBinFile);
			generaH264FileCommand = new Extras.RelayCommand(GeneraH264File, param => this.canGenMP4File);

			selectImagenDeEntradaSalidaCommand = new Extras.RelayCommand(SelectImagenDeEntradaSalida, param => this.CanSelectImagenDeEntradaSalida);
			}

		public void Notify(string propertieName)
			{
			NotifyPropertyChanged(propertieName);
			}

		#region comandos
		public bool CanSelectImagenDeEntradaSalida { get; set; } = true;
		public System.Windows.Input.ICommand selectImagenDeEntradaSalidaCommand { get; }
		public void SelectImagenDeEntradaSalida(object obj)
			{
			string destino = obj as string;

			var dlg = new Microsoft.Win32.OpenFileDialog();

			dlg.Multiselect = false;
			dlg.CheckFileExists = true;
			dlg.CheckPathExists = true;
			dlg.DefaultExt = ".bmp";
			dlg.Title = "Seleccione una imagen bmp / jpg / png";
			dlg.Filter = "Img files (bmp - png - jpg)|*.bmp;*.png;*.jpg|Todos los files (*.*)|*.*";

			if (destino == "Entrada")
				{
				dlg.FileName = string.IsNullOrWhiteSpace(this.lastImagenDeEntrada) ? @"d:\none.jpg" : this.lastImagenDeEntrada;
				dlg.InitialDirectory = System.IO.Path.GetDirectoryName(dlg.FileName);
				}
			else if (destino == "Salida")
				{
				dlg.FileName = string.IsNullOrWhiteSpace(this.lastImagenDeSalida) ? @"d:\none.jpg" : this.lastImagenDeSalida;
				dlg.InitialDirectory = System.IO.Path.GetDirectoryName(dlg.FileName);
				}

			if (dlg.ShowDialog() == true)
				{
				if (destino == "Entrada")
					{
					this.lastImagenDeEntrada = dlg.FileName;
					this.imagenDeEntrada = new System.Windows.Media.Imaging.BitmapImage(new Uri(dlg.FileName));
					}
				else if (destino == "Salida")
					{
					this.lastImagenDeSalida = dlg.FileName;
					this.imagenDeSalida = new System.Windows.Media.Imaging.BitmapImage(new Uri(dlg.FileName));
					}
				}
			}

		public bool CanCloseWindows { get; set; } = true;

		public void CloseMainWindows(object obj)
			{
			mainWindows?.Close();
			}

		public void MinimizeMainWindows(object obj)
			{
			if (mainWindows != null) mainWindows.WindowState = System.Windows.WindowState.Minimized;
			}

		public void MaximizeMainWindows(object obj)
			{
			if (mainWindows != null)
				{
				if (mainWindows.WindowState == System.Windows.WindowState.Maximized)
					{
					mainWindows.WindowState = System.Windows.WindowState.Normal;
					}
				else
					{
					mainWindows.WindowState = System.Windows.WindowState.Maximized;
					}
				}
			}

		public System.Windows.Input.ICommand AppCloseCommand { get; }
		public System.Windows.Input.ICommand AppMinimizeCommand { get; }
		public System.Windows.Input.ICommand AppMaximizeCommand { get; }

		public System.Windows.Input.ICommand saveCommand { get; }

		public void Save(object obj)
			{
			model?.Save();
			configChange = false;
			originalChange = false;
			}

		public System.Windows.Input.ICommand reloadCommand { get; }

		public void Reload(object obj)
			{
			model?.Load();
			configChange = false;
			originalChange = false;

			Notify("host");
			Notify("portHTTP");
			Notify("portRTSP");
			Notify("usuario");
			Notify("password");
			Notify("stringDeConexion");
			Notify("parametrosExtra");
			Notify("urlDeConexion");
			Notify("lastBinFile");
			}

		public bool canOpenBinFile { get; set; } = true;
		public System.Windows.Input.ICommand selectBinFile0Command { get; }

		public void SelectBinFile0(object obj)
			{
			var dlg = new Microsoft.Win32.OpenFileDialog();
			dlg.Title = "Seleccione el fichero binario 00000000";
			dlg.FileName = this.lastBinFile;
			dlg.InitialDirectory = System.IO.Path.GetDirectoryName(dlg.FileName);

			dlg.Multiselect = false;
			dlg.CheckFileExists = true;
			dlg.CheckPathExists = true;

			dlg.DefaultExt = "." + System.IO.Path.GetExtension(dlg.FileName); ;
			dlg.Filter = "Bin files *_00000000.0000.bin|*_00000000.0000.bin|Todos los ficheros *.*|*.*";

			if (dlg.ShowDialog() == true)
				{
				this.lastBinFile = dlg.FileName;
				this.model.Save();
				}
			}

		public bool canGenMP4File { get; set; } = true;
		public System.Windows.Input.ICommand generaH264FileCommand { get; }

		public void GeneraH264File(object obj)
			{
			if (!System.IO.File.Exists(this.lastBinFile))
				{
				System.Windows.MessageBox.Show("El file de existir...", "Operacion Cancelada");
				return;
				}

			string directorio = System.IO.Path.GetDirectoryName(this.lastBinFile);
			string file = System.IO.Path.GetFileName(this.lastBinFile);
			string searchPath = file.Substring(0, file.Length - 17) + "????????.????.bin";
			string[] files = System.IO.Directory.GetFiles(directorio, searchPath);

			if (!System.IO.Directory.Exists(@"D:\LogFiles\ThinRDP\h264_mp4_Files\"))
				{
				System.IO.Directory.CreateDirectory(@"D:\LogFiles\ThinRDP\h264_mp4_Files\");
				}

			System.IO.BinaryWriter bw = new System.IO.BinaryWriter(new System.IO.FileStream(@$"D:\LogFiles\ThinRDP\h264_mp4_Files\{file}.h264", System.IO.FileMode.Create, System.IO.FileAccess.Write));
			foreach (string binFile in files)
				{
				System.IO.BinaryReader br = new System.IO.BinaryReader(new System.IO.FileStream(binFile, System.IO.FileMode.Open, System.IO.FileAccess.Read));
				byte[] data = br.ReadBytes((int)br.BaseStream.Length);
				if (data[0] != 0)
					{
					bw.Write(data);
					}


				br.Close();
				}

			bw.Close();

			this.ffmpegCommandLine = $"ffmpeg -framerate 25 -i {file}.h264 -c:v libx264 -crf 23 -profile:v baseline -level 3.0 -r 30 -pix_fmt yuv420p {file}.mp4";
			this.ffmpegCommandLine = $"x264 -o x264_out.mp4 192%2E168%2E0%2E101_0_00000000.bin.h264";
			this.ffmpegCommandLine = $"x264 --profile high444 -o x264_out.high444.mp4 192%2E168%2E0%2E101_0_00000000.bin.h264";
			this.ffmpegCommandLine = $"x264 --profile high444 --input-csp yuv444 -o x264_out.high444.yuv444.mp4 192%2E168%2E0%2E101_0_00000000.bin.h264";
			this.ffmpegCommandLine = $"x264 --profile high444 --input-csp yuv444 --output-csp i444 -o x264_out.high444.yuv444.i444.mp4 192%2E168%2E0%2E101_0_00000000.bin.h264";
			this.ffmpegCommandLine = $"ffmpeg -framerate 25 -i {file}.h264 -c:v libx264 -crf 23  -level 3.0 -r 25 -vcodec copy {file}.mp4";
			this.ffmpegCommandLine = $"ffmpeg -framerate 25 -i {file}.h264 -level 3.0 -r 25 -vcodec copy {file}.mp4";
			}

		public bool canSelectChunk { get; set; } = true;
		public System.Windows.Input.ICommand selectChunkCommand { get; }

		public void SelectChunk(string selChunkFile)
			{
			System.IO.BinaryReader br = new System.IO.BinaryReader(new System.IO.FileStream(selChunkFile, System.IO.FileMode.Open, System.IO.FileAccess.Read));
			byte[] data = br.ReadBytes((int)br.BaseStream.Length);
			br.Close();

			// ChunkToRichH264(mainWindows.rtbFirstChunk, data);
			}

		public bool CanStartServer { get; set; } = true;
		public System.Windows.Input.ICommand startServerCommand { get; }

		public void StartServer(object obj)
			{
			CanStopServer = true;
			CanStartServer = false;

			//if (this.serverSocket != null)
			//	{
			//	this.serverSocket.Close();
			//	this.serverSocket = null;
			//	}
			}

		public bool CanStopServer { get; set; } = false;
		public System.Windows.Input.ICommand stopServerCommand { get; }

		public void StopServer(object obj)
			{
			CanStopServer = false;
			CanStartServer = true;
			}

		public bool configChange { get; set; } = false;
		public bool originalChange { get; set; } = false;

		public bool canConfigTextChange { get; set; } = true;
		public System.Windows.Input.ICommand configTextChangeCommand { get; }

		public void ConfigTextChange(object obj)
			{
			configChange = true;
			}

		#endregion comandos

		#region propiedades
		public string lastBinFile
			{
			get => model.lastBinFile;
			set { if (model.lastBinFile != value) { model.lastBinFile = value; NotifyPropertyChanged(); } }
			}

		public string lastImagenDeEntrada
			{
			get => model.lastImagenDeEntrada;
			set { if (model.lastImagenDeEntrada != value) { model.lastImagenDeEntrada = value; NotifyPropertyChanged(); } }
			}

		public string lastImagenDeSalida
			{
			get => model.lastImagenDeSalida;
			set { if (model.lastImagenDeSalida != value) { model.lastImagenDeSalida = value; NotifyPropertyChanged(); } }
			}

		public string ffmpegCommandLine
			{
			get => model.ffmpegCommandLine;
			set { if (model.ffmpegCommandLine != value) { model.ffmpegCommandLine = value; NotifyPropertyChanged(); } }
			}

		System.Net.Sockets.TcpListener serverSocketRTSP_v = null;
		public System.Net.Sockets.TcpListener serverSocketRTSP
			{
			get => serverSocketRTSP_v;
			set { if (serverSocketRTSP_v != value) { serverSocketRTSP_v = value; NotifyPropertyChanged(); } }
			}

		List<System.Net.Sockets.Socket> listaDeClientesRTSP_v = null;
		public List<System.Net.Sockets.Socket> listaDeClientesRTSP
			{
			get => listaDeClientesRTSP_v;
			set { if (listaDeClientesRTSP_v != value) { listaDeClientesRTSP_v = value; NotifyPropertyChanged(); } }
			}

		int binFileLength_v = 0;
		public int binFileLength
			{
			get => binFileLength_v;
			set { if (binFileLength_v != value) { binFileLength_v = value; NotifyPropertyChanged(); } }
			}

		int binFilePos_v = 0;
		public int binFilePos
			{
			get => binFilePos_v;
			set { if (binFilePos_v != value) { binFilePos_v = value; NotifyPropertyChanged(); } }
			}

		string binTextPreImagen_v;
		public string binTextPreImagen
			{
			get => binTextPreImagen_v;
			set { if (binTextPreImagen_v != value) { binTextPreImagen_v = value; NotifyPropertyChanged(); } }
			}

		byte[] binFileData_v = null;
		public byte[] binFileData
			{
			get => binFileData_v;
			set { if (binFileData_v != value) { binFileData_v = value; NotifyPropertyChanged(); } }
			}

		private System.Windows.Media.Imaging.BitmapImage ImageData_v;
		public System.Windows.Media.Imaging.BitmapImage ImageData
			{
			get { return this.ImageData_v; }
			set { this.ImageData_v = value; }
			}

		System.Windows.Media.Imaging.BitmapSource imagenMain_v;
		public System.Windows.Media.Imaging.BitmapSource imagenMain
			{
			get
				{
				return imagenMain_v;
				}

			set
				{
				imagenMain_v = value;
				NotifyPropertyChanged();
				}
			}

		#endregion propiedades

		#region metodos
		private static System.Windows.Media.Imaging.BitmapImage LoadImage(byte[] imageData)
			{
			if (imageData == null || imageData.Length == 0) return null;
			var image = new System.Windows.Media.Imaging.BitmapImage();
			using (var mem = new System.IO.MemoryStream(imageData))
				{
				mem.Position = 0;
				mem.Seek(0, System.IO.SeekOrigin.Begin);

				image.BeginInit();
				// image.CreateOptions = System.Windows.Media.Imaging.BitmapCreateOptions.PreservePixelFormat;
				// image.CacheOption = System.Windows.Media.Imaging.BitmapCacheOption.OnLoad;
				// image.UriSource = null;
				image.StreamSource = mem;
				image.EndInit();
				}
			// image.Freeze();
			return image;
			}

		public static System.Windows.Media.ImageSource ByteToImage(byte[] imageData)
			{
			System.Windows.Media.Imaging.BitmapImage biImg = new System.Windows.Media.Imaging.BitmapImage();
			System.IO.MemoryStream ms = new System.IO.MemoryStream(imageData);
			biImg.BeginInit();
			biImg.StreamSource = ms;
			biImg.EndInit();

			System.Windows.Media.ImageSource imgSrc = biImg as System.Windows.Media.ImageSource;

			return imgSrc;
			}

		public static System.Windows.Media.Imaging.BitmapImage ByteToImageBMI(byte[] imageData)
			{
			System.Windows.Media.Imaging.BitmapImage biImg = new System.Windows.Media.Imaging.BitmapImage();
			System.IO.MemoryStream ms = new System.IO.MemoryStream(imageData);
			biImg.BeginInit();
			biImg.StreamSource = ms;
			biImg.EndInit();

			return biImg;
			//System.Windows.Media.ImageSource imgSrc = biImg as System.Windows.Media.ImageSource;

			//return imgSrc;
			}
		#endregion	metodos

		}
	}
