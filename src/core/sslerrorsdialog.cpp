/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "sslerrorsdialog.h"
#include <QDateTime>

namespace LeechCraft
{
SslErrorsDialog::SslErrorsDialog (const QString& url,
		const QList<QSslError>& errors,
		QWidget *parent)
: QDialog (parent)
{
	Ui_.setupUi (this);

	Ui_.UrlEdit_->setText (url);

	for (const auto& err : errors)
		PopulateTree (err);
	Ui_.Errors_->expandAll ();
}

SslErrorsDialog::RememberChoice SslErrorsDialog::GetRememberChoice () const
{
	if (Ui_.RememberNot_->isChecked ())
		return RememberChoice::Not;
	else if (Ui_.RememberFile_->isChecked ())
		return RememberChoice::File;
	else
		return RememberChoice::Host;
}

void SslErrorsDialog::PopulateTree (const QSslError& error)
{
	QTreeWidgetItem *item = new QTreeWidgetItem (Ui_.Errors_,
			QStringList ("Error:") << error.errorString ());

	QSslCertificate cer = error.certificate ();
	if (cer.isNull ())
	{
		new QTreeWidgetItem (item,
				QStringList (tr ("Certificate")) <<
					tr ("(No certificate available for this error)"));
		return;
	}

	new QTreeWidgetItem (item, QStringList (tr ("Valid:")) <<
#if QT_VERSION < 0x050000
				(cer.isValid () ? tr ("yes") : tr ("no")));
#else
				(!cer.isBlacklisted () ? tr ("yes") : tr ("no")));
#endif
	new QTreeWidgetItem (item, QStringList (tr ("Effective date:")) <<
				cer.effectiveDate ().toString ());
	new QTreeWidgetItem (item, QStringList (tr ("Expiry date:")) <<
				cer.expiryDate ().toString ());
	new QTreeWidgetItem (item, QStringList (tr ("Version:")) <<
				cer.version ());
	new QTreeWidgetItem (item, QStringList (tr ("Serial number:")) <<
				cer.serialNumber ());
	new QTreeWidgetItem (item, QStringList (tr ("MD5 digest:")) <<
				cer.digest ().toHex ());
	new QTreeWidgetItem (item, QStringList (tr ("SHA1 digest:")) <<
				cer.digest (QCryptographicHash::Sha1).toHex ());

	QTreeWidgetItem *issuer = new QTreeWidgetItem (item,
			QStringList (tr ("Issuer info")));

	QString tmpString;
#if QT_VERSION >= 0x050000
	auto cvt = [] (const QStringList& list) { return list.join ("; "); };
#else
	auto cvt = [] (const QString& str) { return str; };
#endif
	tmpString = cvt (cer.issuerInfo (QSslCertificate::Organization));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (issuer,
				QStringList (tr ("Organization:")) << tmpString);

	tmpString = cvt (cer.issuerInfo (QSslCertificate::CommonName));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (issuer,
				QStringList (tr ("Common name:")) << tmpString);

	tmpString = cvt (cer.issuerInfo (QSslCertificate::LocalityName));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (issuer,
				QStringList (tr ("Locality:")) << tmpString);

	tmpString = cvt (cer.issuerInfo (QSslCertificate::OrganizationalUnitName));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (issuer,
				QStringList (tr ("Organizational unit name:")) << tmpString);

	tmpString = cvt (cer.issuerInfo (QSslCertificate::CountryName));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (issuer,
				QStringList (tr ("Country name:")) << tmpString);

	tmpString = cvt (cer.issuerInfo (QSslCertificate::StateOrProvinceName));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (issuer,
				QStringList (tr ("State or province name:")) << tmpString);

	QTreeWidgetItem *subject = new QTreeWidgetItem (item,
			QStringList (tr ("Subject info")));

	tmpString = cvt (cer.subjectInfo (QSslCertificate::Organization));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (subject,
				QStringList (tr ("Organization:")) << tmpString);

	tmpString = cvt (cer.subjectInfo (QSslCertificate::CommonName));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (subject,
				QStringList (tr ("Common name:")) << tmpString);

	tmpString = cvt (cer.subjectInfo (QSslCertificate::LocalityName));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (subject,
				QStringList (tr ("Locality:")) << tmpString);

	tmpString = cvt (cer.subjectInfo (QSslCertificate::OrganizationalUnitName));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (subject,
				QStringList (tr ("Organizational unit name:")) << tmpString);

	tmpString = cvt (cer.subjectInfo (QSslCertificate::CountryName));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (subject,
				QStringList (tr ("Country name:")) << tmpString);

	tmpString = cvt (cer.subjectInfo (QSslCertificate::StateOrProvinceName));
	if (!tmpString.isEmpty ())
		new QTreeWidgetItem (subject,
				QStringList (tr ("State or province name:")) << tmpString);
}
}
