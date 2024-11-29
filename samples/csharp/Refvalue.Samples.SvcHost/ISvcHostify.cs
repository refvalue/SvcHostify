using System.Runtime.InteropServices;

namespace Refvalue.Samples.SvcHost
{
    /// <summary>
    /// The main routine of the service.
    /// </summary>
    [ComVisible(true)]
    [Guid("CB62E85F-0C69-C76B-E955-655E0D184E5A")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISvcHostify
    {
        /// <summary>
        /// The service entry point.
        /// </summary>
        /// <param name="args">The input arguments</param>
        void Run(string[] args);

        /// <summary>
        /// This function will be called by the SvcHostify routine in another thread and
        /// you can, for example, send a signal to your 'Run' routine to stop gracefully.
        /// </summary>
        void OnStop();
    }
}
