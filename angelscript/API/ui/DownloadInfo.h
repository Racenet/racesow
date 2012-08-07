/**
 * DownloadInfo
 */
class DownloadInfo
{
public:
	/* object properties */

	/* object behaviors */
	void _beh_1_();&s
	void _beh_0_();&s
	void _beh_0_(const DownloadInfo&in);&s

	/* object methods */
	DownloadInfo& opAssign(const DownloadInfo&in);
	float get_percent() const;
	int get_speed() const;
	int get_type() const;
	const String@ get_name() const;
};

