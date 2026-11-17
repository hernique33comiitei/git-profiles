#include "ProviderCli.h"

#include "Secrets.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>

namespace {
// Pinned minimum for direnv — installed together with the app by install.sh.
const char *kDirenvPin = "2.37.1";

int cmpSemver(const QString &a, const QString &b)
{
    const QStringList pa = a.split(QLatin1Char('.'));
    const QStringList pb = b.split(QLatin1Char('.'));
    const int n = qMax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        const int na = i < pa.size() ? pa.at(i).toInt() : 0;
        const int nb = i < pb.size() ? pb.at(i).toInt() : 0;
        if (na != nb) {
            return na < nb ? -1 : 1;
        }
    }
    return 0;
}
} // namespace

ProviderCli::ProviderCli(QObject *parent)
    : QObject(parent)
{
}

QString ProviderCli::expandTilde(const QString &p)
{
    if (p == QLatin1String("~")) {
        return QDir::homePath();
    }
    if (p.startsWith(QLatin1String("~/"))) {
        return QDir::homePath() + p.mid(1);
    }
    return p;
}

QString ProviderCli::stripScheme(const QString &url)
{
    QString u = url.trimmed();
    const int s = u.indexOf(QStringLiteral("://"));
    if (s >= 0) {
        u = u.mid(s + 3);
    }
    while (u.endsWith(QLatin1Char('/'))) {
        u.chop(1);
    }
    return u;
}

QVariantMap ProviderCli::environment() const
{
    const QString direnv = QStandardPaths::findExecutable(QStringLiteral("direnv"));
    bool hook = false;
    QFile bashrc(QDir::homePath() + QStringLiteral("/.bashrc"));
    if (bashrc.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString all = QString::fromUtf8(bashrc.readAll());
        hook = all.contains(QStringLiteral("direnv hook"));
    }
    QString direnvVersion;
    if (!direnv.isEmpty()) {
        QProcess p;
        p.start(direnv, {QStringLiteral("version")});
        if (p.waitForStarted(1500)) {
            p.waitForFinished(3000);
            const QString out = QString::fromUtf8(p.readAllStandardOutput());
            static const QRegularExpression re(QStringLiteral("([0-9]+\\.[0-9]+\\.[0-9]+)"));
            const QRegularExpressionMatch mm = re.match(out);
            if (mm.hasMatch()) {
                direnvVersion = mm.captured(1);
            }
        }
    }
    const QString pin = QString::fromLatin1(kDirenvPin);
    const bool direnvOk = !direnvVersion.isEmpty() && cmpSemver(direnvVersion, pin) >= 0;

    QVariantMap m;
    m[QStringLiteral("direnvInstalled")] = !direnv.isEmpty();
    m[QStringLiteral("direnvVersion")] = direnvVersion;
    m[QStringLiteral("direnvPin")] = pin;
    m[QStringLiteral("direnvOk")] = direnvOk;
    m[QStringLiteral("direnvHook")] = hook;
    m[QStringLiteral("direnvUrl")] = QStringLiteral("https://direnv.net/docs/installation.html");
    m[QStringLiteral("secretTool")] = !Secrets::secretToolPath().isEmpty();
    return m;
}

QString ProviderCli::defaultGhConfigDir(const QString &host) const
{
    return QStringLiteral("~/.config/gh-profiles/") + host;
}

QString ProviderCli::homeDir() const
{
    return QDir::homePath();
}

QString ProviderCli::envrc(const QVariantMap &profile) const
{
    const QString provider = profile.value(QStringLiteral("provider")).toString();
    const QString host = profile.value(QStringLiteral("host")).toString();
    const QString hostName = profile.value(QStringLiteral("hostName")).toString();
    const QString apiUrl = profile.value(QStringLiteral("apiUrl")).toString();
    const QString owner = profile.value(QStringLiteral("owner")).toString();
    QString ghConfig = profile.value(QStringLiteral("ghConfig")).toString();
    if (ghConfig.isEmpty()) {
        ghConfig = defaultGhConfigDir(host);
    }

    QStringList out;
    out << QStringLiteral("# Managed by git-profiles — profile \"%1\"").arg(host);

    if (provider == QLatin1String("github")) {
        // `~` is not expanded inside double quotes in bash; use $HOME.
        QString ghc = ghConfig;
        if (ghc == QLatin1String("~")) {
            ghc = QStringLiteral("$HOME");
        } else if (ghc.startsWith(QLatin1String("~/"))) {
            ghc = QStringLiteral("$HOME") + ghc.mid(1);
        }
        out << QStringLiteral("export GH_CONFIG_DIR=\"%1\"").arg(ghc);
    } else if (provider == QLatin1String("gitlab")) {
        QString h = stripScheme(apiUrl);
        if (h.isEmpty()) {
            h = hostName.isEmpty() ? QStringLiteral("gitlab.com") : hostName;
        }
        out << QStringLiteral("export GITLAB_HOST=\"%1\"").arg(h);
        out << QStringLiteral(
                   "export GITLAB_TOKEN=\"$(secret-tool lookup service gitprofiles profile %1 "
                   "2>/dev/null)\"")
                   .arg(host);
    } else if (provider == QLatin1String("gitea")) {
        QString url = apiUrl;
        if (url.isEmpty() && !hostName.isEmpty()) {
            url = QStringLiteral("https://") + hostName;
        }
        if (!url.isEmpty()) {
            out << QStringLiteral("export GITEA_CLI_HOST=\"%1\"").arg(url);
        }
        if (!owner.isEmpty()) {
            out << QStringLiteral("export GITEA_CLI_USER=\"%1\"").arg(owner);
        }
        out << QStringLiteral(
                   "export GITEA_TOKEN=\"$(secret-tool lookup service gitprofiles profile %1 "
                   "2>/dev/null)\"")
                   .arg(host);
    } else {
        out << QStringLiteral("# No provider selected — SSH identity only.");
    }

    return out.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

QString ProviderCli::writeEnvrc(const QString &dir, const QVariantMap &profile)
{
    const QString base = expandTilde(dir);
    if (!QDir(base).exists()) {
        return QStringLiteral("Directory does not exist: %1").arg(base);
    }
    const QString path = base + QStringLiteral("/.envrc");
    const QString begin = QStringLiteral("# >>> git-profiles >>>");
    const QString end = QStringLiteral("# <<< git-profiles <<<");
    const QString block = begin + QLatin1Char('\n') + envrc(profile) + end + QLatin1Char('\n');

    QString existing;
    QFile in(path);
    if (in.exists() && in.open(QIODevice::ReadOnly | QIODevice::Text)) {
        existing = QString::fromUtf8(in.readAll());
        in.close();
    }

    QString result;
    const int b = existing.indexOf(begin);
    const int e = existing.indexOf(end);
    if (b >= 0 && e > b) {
        const int after = e + end.size();
        result = existing.left(b) + block.trimmed() + existing.mid(after);
    } else if (existing.isEmpty()) {
        result = block;
    } else {
        result = existing;
        if (!result.endsWith(QLatin1Char('\n'))) {
            result += QLatin1Char('\n');
        }
        result += QLatin1Char('\n') + block;
    }

    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QStringLiteral("Cannot write %1: %2").arg(path, out.errorString());
    }
    out.write(result.toUtf8());
    if (!out.commit()) {
        return QStringLiteral("Cannot write %1: %2").arg(path, out.errorString());
    }

    const QString direnv = QStandardPaths::findExecutable(QStringLiteral("direnv"));
    if (!direnv.isEmpty()) {
        QProcess p;
        p.start(direnv, {QStringLiteral("allow"), base});
        p.waitForFinished(5000);
    }
    return {};
}

QString ProviderCli::envrcStatus(const QString &dir, const QVariantMap &profile) const
{
    const QString path = expandTilde(dir) + QStringLiteral("/.envrc");
    QFile f(path);
    if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("absent");
    }
    const QString all = QString::fromUtf8(f.readAll());
    const QString begin = QStringLiteral("# >>> git-profiles >>>");
    const QString end = QStringLiteral("# <<< git-profiles <<<");
    const int b = all.indexOf(begin);
    const int e = all.indexOf(end);
    if (b < 0 || e <= b) {
        return QStringLiteral("absent");
    }
    const QString inner = all.mid(b + begin.size(), e - (b + begin.size()));
    return inner.trimmed() == envrc(profile).trimmed() ? QStringLiteral("ok")
                                                       : QStringLiteral("outdated");
}

QString ProviderCli::setupCredential(const QVariantMap &profile, const QString &token)
{
    const QString host = profile.value(QStringLiteral("host")).toString();
    const QString provider = profile.value(QStringLiteral("provider")).toString();
    if (host.isEmpty()) {
        return QStringLiteral("Missing profile.");
    }
    if (token.isEmpty()) {
        return QStringLiteral("Paste a personal access token first.");
    }

    // Always keep the token in the keyring (gitlab/gitea .envrc read it back).
    Secrets secrets;
    const QString secErr = secrets.setToken(host, token);
    if (!secErr.isEmpty()) {
        return secErr;
    }

    const QString hostName = profile.value(QStringLiteral("hostName")).toString();
    const QString apiUrl = profile.value(QStringLiteral("apiUrl")).toString();

    auto runWithToken = [&token](const QString &program, const QStringList &args,
                                 const QProcessEnvironment &env, QString *err) -> bool {
        const QString exe = QStandardPaths::findExecutable(program);
        if (exe.isEmpty()) {
            return true; // CLI absent: token is in the keyring, nothing else to do
        }
        QProcess p;
        if (!env.isEmpty()) {
            p.setProcessEnvironment(env);
        }
        p.start(exe, args);
        if (!p.waitForStarted(2000)) {
            *err = QStringLiteral("Could not run %1").arg(program);
            return false;
        }
        p.write(token.toUtf8());
        p.write("\n");
        p.closeWriteChannel();
        p.waitForFinished(15000);
        if (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0) {
            return true;
        }
        *err = QString::fromUtf8(p.readAllStandardError()).trimmed();
        return false;
    };

    QString err;
    if (provider == QLatin1String("github")) {
        QString ghConfig = profile.value(QStringLiteral("ghConfig")).toString();
        if (ghConfig.isEmpty()) {
            ghConfig = defaultGhConfigDir(host);
        }
        const QString dir = expandTilde(ghConfig);
        QDir().mkpath(dir);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("GH_CONFIG_DIR"), dir);
        QString h = stripScheme(apiUrl);
        if (h.isEmpty()) {
            h = QStringLiteral("github.com");
        }
        if (!runWithToken(QStringLiteral("gh"),
                          {QStringLiteral("auth"), QStringLiteral("login"),
                           QStringLiteral("--hostname"), h, QStringLiteral("--with-token")},
                          env, &err)) {
            return QStringLiteral("Token saved, but gh login failed: %1").arg(err);
        }
    } else if (provider == QLatin1String("gitlab")) {
        QString h = stripScheme(apiUrl);
        if (h.isEmpty()) {
            h = hostName.isEmpty() ? QStringLiteral("gitlab.com") : hostName;
        }
        if (!runWithToken(QStringLiteral("glab"),
                          {QStringLiteral("auth"), QStringLiteral("login"),
                           QStringLiteral("--hostname"), h, QStringLiteral("--stdin")},
                          QProcessEnvironment(), &err)) {
            return QStringLiteral("Token saved, but glab login failed: %1").arg(err);
        }
    } else if (provider == QLatin1String("gitea")) {
        QString url = apiUrl;
        if (url.isEmpty() && !hostName.isEmpty()) {
            url = QStringLiteral("https://") + hostName;
        }
        if (!url.isEmpty()
            && !runWithToken(QStringLiteral("gt"),
                             {QStringLiteral("auth"), QStringLiteral("login"),
                              QStringLiteral("--host"), url, QStringLiteral("--with-token")},
                             QProcessEnvironment(), &err)) {
            return QStringLiteral("Token saved, but gt login failed: %1").arg(err);
        }
    }

    return {};
}
