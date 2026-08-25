const __base = String(window.AIFRED_API_BASE_URL || window.location.origin).replace(/\/+$/, "");
const __apiV1Base = __base.endsWith("/api/v1") || __base.endsWith("/v1") ? __base : `${__base}/api/v1`;
window.AIFRED_CONFIG = {
  apiBase: __base,
  apiV1Base: __apiV1Base,
  contactEmail: "north3rnlight3rofficial@outlook.com",
  downloadUrls: {
    windowsInstaller: `${__apiV1Base}/downloads/plugin?asset=setup`,
    windowsZip: `${__apiV1Base}/downloads/plugin?asset=zip`,
    releaseNotes: `${__base}/assets/docs/aifred-release-notes.txt`
  },
  productPrice: "Free beta download"
};
