#ifndef STRUCTURES_H
#define STRUCTURES_H

class QMenu;

namespace LeechCraft
{
	/** @brief Describes single task parameter.
	 */
	enum TaskParameter
	{
		/** Use default parameters.
		 */
		NoParameters = 0,
		/** Task should not be started automatically after addition.
		 */
		NoAutostart = 1,
		/** Task should not be saved in history.
		 */
		DoNotSaveInHistory = 2,
		/** Task is created as a result of user's actions.
		 */
		FromUserInitiated = 8,
		/** User should not be notified about task finish.
		 */
		DoNotNotifyUser = 32,
		/** Task is used internally and would not be visible to the user
		 * at all.
		 */
		Internal = 64,
		/** Task should not be saved as it would have no meaning after
		 * next start.
		 */
		NotPersistent = 128,
		/** When the task is finished, it should not be announced via
		 * gotEntity() signal.
		 */
		DoNotAnnounceEntity = 256
	};

	Q_DECLARE_FLAGS (TaskParameters, TaskParameter);

	/** @brief Describes parameters of a download job.
	 *
	 * Describes where and what should be saved.
	 *
	 * @sa LeechCraft::TaskParameter
	 */
	struct DownloadEntity
	{
		/** @brief What the user wants to download - this could be
		 * anything from URL to Magnet hash to local torrent file.
		 */
		QByteArray Entity_;
		/** @brief Where it wants the data to be saved.
		 */
		QString Location_;
		QString Mime_;
		TaskParameters Parameters_;
	};

	enum CustomDataRoles
	{
		/** The role for the string list with tags. So, QStringList is
		 * expected to be returned.
		 */
		RoleTags = 100,
		/* The role for the additional controls for a given item.
		 * QWidget* is expected to be returned.
		 */
		RoleControls,
		/** The role for the widget appearing on the right part of the
		 * screen when the user selects an item. QWidget* is expected to
		 * be returned.
		 */
		RoleAdditionalInfo,
		/** The role for the hash of the item, used to compare two
		 * different results, possibly from two different models.
		 * QByteArray is expected to be returned.
		 */
		RoleHash,
		/** This should return MIME of an item if it's available,
		 * otherwise an empty string should be returned.
		 */
		RoleMime
	};
};

Q_DECLARE_OPERATORS_FOR_FLAGS (LeechCraft::TaskParameters);

#endif

