using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace EncoderH264_test
	{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
		{
		ViewModel.MainViewModel viewModel;

		static Microsoft.Win32.RegistryKey appByOJChKey = Microsoft.Win32.Registry.CurrentUser.CreateSubKey("EncoderH264_test");
		bool inicializando { get; set; } = true;

		public MainWindow()
			{
			ViewModel.MainViewModel.appByJOChKey = MainWindow.appByOJChKey;
			ViewModel.MainViewModel.mainWindows = this;

			InitializeComponent();

			if ((DataContext != null) && (DataContext is ViewModel.MainViewModel))
				{
				viewModel = DataContext as ViewModel.MainViewModel;
				}
			else
				{
				DataContext = viewModel = new ViewModel.MainViewModel();
				}

			inicializando = false;
			}

		private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
			{
			if (viewModel?.CanStopEncoder == true) viewModel?.stopEncoderCommand.Execute(null);
			}
		}
	}
